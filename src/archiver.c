#include "metadata.h"
#include "archiver.h"

#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path, mode) _mkdir(path)
#else
    #include <sys/stat.h>
    #define MKDIR(path, mode) mkdir(path, mode)
#endif

int create_archive(const char *archive_path, const char *dir_path) {
    archive_header_t header;
    init_header(&header);

    FILE *arch = fopen(archive_path, "wb");
    if (!arch) {
        fprintf(stderr, "Failed to create archive: %s\n", archive_path);
        perror("Error");
        return 2;
    }

    uint64_t entry_count = scan_entry_count(dir_path);

    if (entry_count == 0) {
        fprintf(stderr, "No files to archive in directory: %s\n", dir_path);
        fclose(arch);
        return 3;
    } if (entry_count > 10000) {
        printf("Warning: Archiving a large number of files (%lu). This may take some time.\n", entry_count);
    }

    header.data_table_offset = (sizeof(archive_header_t) + (sizeof(archive_file_entry_t) * entry_count));

    uint64_t *entry_pos = malloc(sizeof(uint64_t)); // for global tracking of file entry position
    *entry_pos = header.file_table_offset;
    
    fseek(arch, header.data_table_offset, SEEK_SET);
    int result = write_and_create_entry_dir(&header, arch, entry_pos, dir_path, "");

    fseek(arch, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, arch);

    fclose(arch);
    free(entry_pos);

    return result;
}

int write_and_create_entry_dir(archive_header_t *header, FILE *archive_file, uint64_t *entry_pos, const char *path, const char *rel_path) {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", path);
        perror("Error");
        return 3;
    }

    while (( entry = readdir(dir) ) != NULL) {
        char full_path[1024];
        char relative_path[MAX_FILENAME_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if(stat(full_path, &file_stat) == 0) {
            if (S_ISREG(file_stat.st_mode)) {
                // Create relative path for the file entry
                snprintf(relative_path, sizeof(relative_path), "%s%s", 
                        rel_path, entry->d_name);
                
                FILE *afile = fopen(full_path, "rb");
                if (!afile) {
                    fprintf(stderr, "Failed to open file: %s\n", full_path);
                    perror("Error");
                    closedir(dir);
                    return 2;
                }

                archive_file_entry_t file_entry;
                strncpy(file_entry.filename, relative_path, MAX_FILENAME_LENGTH - 1);
                file_entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
                file_entry.id = header->file_count;
                file_entry.offset = ftell(archive_file);
                file_entry.size = file_stat.st_size;
                file_entry.type = 0;

                uint8_t  buffer[4096];
                uint32_t bytes_read;

                while ( (bytes_read = fread(buffer, 1, sizeof(buffer), afile)) > 0) {
                    if (fwrite(buffer, 1, bytes_read, archive_file) != bytes_read) {
                        fclose(afile);
                        closedir(dir);
                        return 4;
                    }
                }

                fclose(afile);

                fseek(archive_file, *entry_pos, SEEK_SET); // Move to the position of the file entry
                fwrite(&file_entry, sizeof(file_entry), 1, archive_file); // Write the file entry
                *entry_pos = ftell(archive_file); // Update the position for the next entry
                fseek(archive_file, file_entry.offset + file_entry.size, SEEK_SET); // Move back to the end of the data table for the next file
            } else {
                // Create relative path for the directory entry
                snprintf(relative_path, sizeof(relative_path), "%s%s/", 
                        rel_path, entry->d_name);

                archive_file_entry_t file_entry;
                strncpy(file_entry.filename, relative_path, MAX_FILENAME_LENGTH - 1);
                file_entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
                file_entry.id = header->file_count;
                file_entry.offset = 0;
                file_entry.size = 0;
                file_entry.type = 1;

                uint64_t pos = ftell(archive_file); // Save current position to return after writing the entry

                fseek(archive_file, *entry_pos, SEEK_SET);
                fwrite(&file_entry, sizeof(file_entry), 1, archive_file);
                *entry_pos = ftell(archive_file);
                fseek(archive_file, pos, SEEK_SET);

                int result = write_and_create_entry_dir(header, archive_file, entry_pos, full_path, relative_path);

                if (result != 0){
                    closedir(dir);
                    return result;
                }
            }

            header->file_count++;
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

int extract_archive(const char *archive_path, const char *dst_dir) {
    create_directory_recursive(dst_dir);
    
    FILE *arch = fopen(archive_path, "rb");
    if (!arch) {
        fprintf(stderr, "Failed to open archive: %s\n", archive_path);
        perror("Error");
        return 2;
    }

    archive_header_t header;

    fread(&header, sizeof(header), 1, arch);

    if (memcmp(header.magic, MAGIC, 4) != 0) {
        fclose(arch);
        fprintf(stderr, "Unknown magic in: %s\n", archive_path);
        return 6;
    }

    int result = extract_entry(&header, arch, dst_dir);

    fclose(arch);

    return result;
}

int extract_entry(archive_header_t *header, FILE *archive_file, const char *dst_dir) {
    int result;
    
    result = create_dirs(header, archive_file, dst_dir);
    if (result != 0) {
        return result;
    }

    result = write_files(header, archive_file, dst_dir);
    if (result != 0) {
        return result;
    }

    return 0;
}

int create_dirs(archive_header_t *header, FILE *archive_file, const char *dst_dir) {
    fseek(archive_file, header->file_table_offset, SEEK_SET);

    for (uint64_t i = 0; i < header->file_count; i++) {
        archive_file_entry_t file_entry;
        fread(&file_entry, sizeof(file_entry), 1, archive_file);

        // Means it's a directory
        if (file_entry.type == 1) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dst_dir, file_entry.filename);

            create_directory_recursive(full_path);
        }
    }

    return 0;
}

int write_files(archive_header_t *header, FILE *archive_file, const char *dst_dir) {
    fseek(archive_file, header->file_table_offset, SEEK_SET);

    for (uint64_t i = 0; i < header->file_count; i++) {
        archive_file_entry_t file_entry;
        fread(&file_entry, sizeof(file_entry), 1, archive_file);

        if (file_entry.type == 0) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dst_dir, file_entry.filename);
            
            FILE *ofile = fopen(full_path, "wb");
            if (!ofile) {
                fprintf(stderr, "Failed to open file: %s\n", full_path);
                perror("Error");
                return 2;
            }

            uint64_t current_pos = ftell(archive_file);

            fseek(archive_file, file_entry.offset, SEEK_SET);

            uint8_t  buffer[4096];
            uint64_t bytes_remain = file_entry.size;

            while (bytes_remain > 0) {
                uint32_t bytes_to_read = (bytes_remain < sizeof(buffer)) ? bytes_remain : sizeof(buffer);
                uint32_t bytes_read = fread(buffer, 1, bytes_to_read, archive_file);

                if (bytes_read == 0) {
                    break;
                }

                fwrite(buffer, 1, bytes_read, ofile);
                bytes_remain -= bytes_read;
            }

            fclose(ofile);

            fseek(archive_file, current_pos, SEEK_SET);
        }
    }

    return 0;
}

FILE *load_archive(const char *archive_path) {
    FILE *arch = fopen(archive_path, "r+b");
    if (!arch) {
        fprintf(stderr, "Failed to open archive: %s\n", archive_path);
        perror("Error");
        return NULL;
    }

    return arch;
}

archive_file_entry_t *get_entries(FILE *archive_file, uint64_t *count) {
    archive_header_t header;
    fseek(archive_file, 0, SEEK_SET);
    fread(&header, sizeof(header), 1, archive_file);

    if (memcmp(header.magic, MAGIC, 4) != 0) {
        fprintf(stderr, "Unknown magic in archive.\n");
        return NULL;
    }

    archive_file_entry_t *entries = malloc(header.file_count * sizeof(archive_file_entry_t));
    if (!entries) return NULL;

    fseek(archive_file, header.file_table_offset, SEEK_SET);

    fread(entries, sizeof(archive_file_entry_t), header.file_count, archive_file);

    *count = header.file_count;

    return entries;
}

int init_header(archive_header_t *header) {
    header->magic[0] = 'T';
    header->magic[1] = 'E';
    header->magic[2] = 'T';
    header->magic[3] = 'O';
    header->version = VERSION;
    header->file_count = 0;
    header->file_table_offset = sizeof(archive_header_t);
    header->data_table_offset = 0;

    return 0;
}

void create_directory_recursive(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);
    
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            MKDIR(tmp, 0755); // Create parent, ignore errors if exists
            *p = '/';
        }
    }
    MKDIR(tmp, 0755); // Create final directory
}