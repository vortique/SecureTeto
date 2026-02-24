#include "metadata.h"
#include "archiver.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

int create_archive(const char *archive_path, const char *dir_path) {
    archive_header_t header;
    init_header(&header);

    FILE *arch = fopen(archive_path, "wb");
    if (!arch) {
        return 2;
    }

    uint64_t entry_count = scan_entry_count(dir_path);
    header.data_table_offset = (sizeof(archive_header_t) + (sizeof(archive_file_entry_t) * entry_count));

    uint64_t entry_pos = header.file_table_offset;

    fwrite(&header, sizeof(header), 1, arch);
    
    fseek(arch, entry_pos, SEEK_SET);
    int result = write_and_create_entry_dir(&header, arch, entry_pos, dir_path, "");

    fseek(arch, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, arch);

    fclose(arch);

    return result;
}

int write_and_create_entry_dir(archive_header_t *header, FILE *archive_file, uint64_t entry_pos, const char *path, const char *rel_path) {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(path);
    if (!dir) {
        return 3;
    }

    while (( entry = readdir(dir) ) != NULL) {
        char full_path[1024];
        char relative_path[MAX_FILENAME_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if(stat(full_path, &file_stat) == 0) {
            header->file_count++;

            if (S_ISREG(file_stat.st_mode)) {
                snprintf(relative_path, sizeof(relative_path), "%s%s", 
                        rel_path, entry->d_name);
                
                FILE *afile = fopen(full_path, "rb");
                if (!afile) {
                    closedir(dir);
                    return 2;
                }

                archive_file_entry_t file_entry;
                strncpy(file_entry.filename, relative_path, MAX_FILENAME_LENGTH - 1);
                file_entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
                file_entry.offset = ftell(archive_file);
                file_entry.size = file_stat.st_size;

                uint8_t  buffer[4098];
                uint32_t bytes_read;

                while ( (bytes_read = fread(buffer, 1, sizeof(buffer), afile)) > 0) {
                    if (fwrite(buffer, 1, bytes_read, archive_file) != bytes_read) {
                        fclose(afile);
                        closedir(dir);
                        return 4;
                    }
                }

                fclose(afile);

                fseek(archive_file, entry_pos, SEEK_SET);
                fwrite(&file_entry, sizeof(file_entry), 1, archive_file);
                entry_pos = ftell(archive_file);
                fseek(archive_file, file_entry.offset + file_entry.size, SEEK_SET);
            } else {
                snprintf(relative_path, sizeof(relative_path), "%s%s/", 
                        rel_path, entry->d_name);

                archive_file_entry_t file_entry;
                strncpy(file_entry.filename, relative_path, MAX_FILENAME_LENGTH - 1);
                file_entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
                file_entry.offset = 0;
                file_entry.size = 0;

                uint64_t pos = ftell(archive_file);

                fseek(archive_file, entry_pos, SEEK_SET);
                fwrite(&file_entry, sizeof(file_entry), 1, archive_file);
                entry_pos = ftell(archive_file);
                fseek(archive_file, pos, SEEK_SET);

                int result = write_and_create_entry_dir(header, archive_file, entry_pos, full_path, relative_path);

                if (result != 0){
                    closedir(dir);
                    return result;
                }
            }
        }
    }

    closedir(dir);
    return 0;
}

uint64_t scan_entry_count(const char *path) {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(path);
    if (!dir) {
        return 0;
    }

    uint64_t entry_count = 0;

    while (( entry = readdir(dir) ) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                entry_count++;
            } else {
                entry_count++;
                entry_count += scan_entry_count(full_path);
            }
        }
    }

    closedir(dir);
    return entry_count;
}

int init_header(archive_header_t *header) {
    header->magic[0] = 'S';
    header->magic[1] = 'E';
    header->magic[2] = 'C';
    header->magic[3] = 'U';
    header->version = VERSION;
    header->file_count = 0;
    header->file_table_offset = sizeof(archive_header_t);
    header->data_table_offset = 0;

    return 0;
}