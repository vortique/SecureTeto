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

#define CHUNK_SIZE 4096
#define CIPHER_CHUNK (CHUNK_SIZE + MAC_SIZE)

static void derive_nonce(uint8_t out[NONCE_SIZE], const uint8_t master[NONCE_SIZE], uint64_t counter);

int create_archive(const char *archive_path, const char *dir_path, const char *pwd) {
    if (strcmp(archive_path, "") == 0 || strcmp(dir_path, "") == 0 || strcmp(pwd, "") == 0) {
        fprintf(stderr, "Missing arguments.\n");
        return 1;
    }

    archive_header_t header;
    init_header(&header);

    FILE *arch = fopen(archive_path, "wb");
    if (!arch) {
        fprintf(stderr, "Failed to create archive: %s\n", archive_path);
        perror("Error");
        return 2;
    }

    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialize libsodium\n");
        fclose(arch);
        return 8;
    }

    uint8_t salt[SALT_SIZE];
    randombytes_buf(salt, sizeof(salt));

    uint8_t key[KEY_SIZE];
    if (crypto_pwhash(
            key, sizeof(key),
            pwd, strlen(pwd),
            salt,
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_DEFAULT) != 0) {
        fprintf(stderr, "Failed to hash password (out of memory?).\n");
        fclose(arch);
        return 9;
    }

    uint8_t master_nonce[NONCE_SIZE];
    randombytes_buf(master_nonce, NONCE_SIZE);

    sodium_context_t sodium_ctx;
    memset(&sodium_ctx, 0, sizeof(sodium_ctx));
    memcpy(sodium_ctx.key,         key,          KEY_SIZE);
    memcpy(sodium_ctx.nonce,       master_nonce, NONCE_SIZE);
    memcpy(sodium_ctx.entry_nonce, master_nonce, NONCE_SIZE);
    sodium_ctx.entry_nonce[NONCE_SIZE - 1] ^= 0xFF; // separate namespace for entry table
    sodium_ctx.block_counter = 0;
    sodium_ctx.entry_counter = 0;

    fwrite(salt,         1, sizeof(salt), arch);
    fwrite(master_nonce, 1, NONCE_SIZE,   arch);

    uint64_t entry_count = scan_entry_count(dir_path);
    if (entry_count == 0) {
        fprintf(stderr, "No files to archive in directory: %s\n", dir_path);
        fclose(arch);
        return 3;
    }
    if (entry_count > 10000) {
        printf("Warning: Archiving a large number of files (%llu). This may take some time.\n",
               (unsigned long long)entry_count);
    }

    size_t preamble_size = sizeof(salt) + NONCE_SIZE;
    header.file_table_offset = preamble_size + sizeof(archive_header_t);
    header.data_table_offset = header.file_table_offset
                                + (sizeof(archive_file_entry_t) + MAC_SIZE) * entry_count;

    uint64_t entry_pos = header.file_table_offset;

    fseek(arch, header.data_table_offset, SEEK_SET);
    int result = write_and_create_entry_dir(&header, arch, &entry_pos, dir_path, "", &sodium_ctx);

    fseek(arch, sizeof(salt) + NONCE_SIZE, SEEK_SET);
    fwrite(&header, sizeof(header), 1, arch);

    fclose(arch);

    sodium_memzero(key, sizeof(key));
    sodium_memzero(&sodium_ctx, sizeof(sodium_ctx));

    return result;
}

static void derive_nonce(uint8_t out[NONCE_SIZE], const uint8_t master[NONCE_SIZE], uint64_t counter) {
    memcpy(out, master, NONCE_SIZE);
    for (int i = 0; i < 8; i++) {
        out[i] ^= (uint8_t)(counter >> (i * 8));
    }
}

int write_and_create_entry_dir(archive_header_t *header,
                               FILE *archive_file,
                               uint64_t *entry_pos,
                               const char *path,
                               const char *rel_path,
                               sodium_context_t *sodium_ctx) {
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(path);
    if (!dir) {
        fprintf(stderr, "Failed to open directory: %s\n", path);
        perror("Error");
        return 3;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[1024];
        char relative_path[MAX_FILENAME_LENGTH];

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) == 0) {
            header->file_count++;

            if (S_ISREG(file_stat.st_mode)) {
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
                memset(&file_entry, 0, sizeof(file_entry));
                strncpy(file_entry.filename, relative_path, MAX_FILENAME_LENGTH - 1);
                file_entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
                file_entry.id     = header->file_count;
                file_entry.offset = (uint64_t)ftell(archive_file);
                file_entry.size   = 0;
                file_entry.type   = 0;

                uint8_t  buffer[CHUNK_SIZE];
                uint8_t  ciphertext[CIPHER_CHUNK];
                uint64_t encrypted_size = 0;
                uint32_t bytes_read;

                while ((bytes_read = fread(buffer, 1, sizeof(buffer), afile)) > 0) {
                    uint8_t block_nonce[NONCE_SIZE];
                    derive_nonce(block_nonce, sodium_ctx->nonce, sodium_ctx->block_counter++);

                    crypto_secretbox_easy(ciphertext, buffer, bytes_read,
                                          block_nonce, sodium_ctx->key);

                    size_t write_len = bytes_read + MAC_SIZE;
                    if (fwrite(ciphertext, 1, write_len, archive_file) != write_len) {
                        fclose(afile);
                        closedir(dir);
                        return 4;
                    }
                    encrypted_size += write_len;
                }

                fclose(afile);

                file_entry.size = encrypted_size;

                uint64_t data_pos = (uint64_t)ftell(archive_file);
                fseek(archive_file, (long)*entry_pos, SEEK_SET);

                uint8_t entry_nonce[NONCE_SIZE];
                derive_nonce(entry_nonce, sodium_ctx->entry_nonce, sodium_ctx->entry_counter++);

                uint8_t entry_cipher[sizeof(file_entry) + MAC_SIZE];
                crypto_secretbox_easy(entry_cipher, (uint8_t *)&file_entry, sizeof(file_entry),
                                      entry_nonce, sodium_ctx->key);

                fwrite(entry_cipher, 1, sizeof(entry_cipher), archive_file);
                *entry_pos = (uint64_t)ftell(archive_file);
                fseek(archive_file, (long)data_pos, SEEK_SET);

            } else {
                snprintf(relative_path, sizeof(relative_path), "%s%s/",
                         rel_path, entry->d_name);

                archive_file_entry_t file_entry;
                memset(&file_entry, 0, sizeof(file_entry));
                strncpy(file_entry.filename, relative_path, MAX_FILENAME_LENGTH - 1);
                file_entry.filename[MAX_FILENAME_LENGTH - 1] = '\0';
                file_entry.id     = header->file_count;
                file_entry.offset = 0;
                file_entry.size   = 0;
                file_entry.type   = 1;

                uint64_t data_pos = (uint64_t)ftell(archive_file);
                fseek(archive_file, (long)*entry_pos, SEEK_SET);

                uint8_t entry_nonce[NONCE_SIZE];
                derive_nonce(entry_nonce, sodium_ctx->entry_nonce, sodium_ctx->entry_counter++);

                uint8_t entry_cipher[sizeof(file_entry) + MAC_SIZE];
                crypto_secretbox_easy(entry_cipher, (uint8_t *)&file_entry, sizeof(file_entry),
                                      entry_nonce, sodium_ctx->key);

                fwrite(entry_cipher, 1, sizeof(entry_cipher), archive_file);
                *entry_pos = (uint64_t)ftell(archive_file);
                fseek(archive_file, data_pos, SEEK_SET);

                int result = write_and_create_entry_dir(header, archive_file,
                                                        entry_pos, full_path,
                                                        relative_path, sodium_ctx);
                if (result != 0) {
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
    if (!dir) return 0;

    uint64_t entry_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) == 0) {
            entry_count++;
            if (!S_ISREG(file_stat.st_mode)) {
                entry_count += scan_entry_count(full_path);
            }
        }
    }

    closedir(dir);
    return entry_count;
}

int extract_archive(const char *archive_path, const char *dst_dir, const char *pwd) {
    if (strcmp(archive_path, "") == 0 || strcmp(dst_dir, "") == 0 || strcmp(pwd, "") == 0) {
        fprintf(stderr, "Missing arguments.\n");
        return 1;
    }

    create_directory_recursive(dst_dir);

    archive_context_t contex;
    memset(&contex, 0, sizeof(contex));

    if (load_archive(archive_path, "rb", pwd, &contex) != 0) {
        return 1;
    }

    int result = extract_entry(&contex, dst_dir, pwd);
    free_archive(&contex);
    return result;
}

int extract_entry(archive_context_t *contex, const char *dst_dir, const char *pwd) {
    int result;
    result = create_dirs(contex, dst_dir, pwd);
    if (result != 0) return result;
    return write_files(contex, dst_dir);
}

int create_dirs(archive_context_t *contex, const char *dst_dir, const char *pwd) {
    for (uint64_t i = 0; i < contex->header.file_count; i++) {
        if (contex->entries[i].type == 1) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dst_dir, contex->entries[i].filename);
            create_directory_recursive(full_path);
        }
    }
    return 0;
}

int write_files(archive_context_t *contex, const char *dst_dir) {
    for (uint64_t i = 0; i < contex->header.file_count; i++) {
        archive_file_entry_t *fe = &contex->entries[i];
        if (fe->type != 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dst_dir, fe->filename);

        FILE *ofile = fopen(full_path, "wb");
        if (!ofile) {
            fprintf(stderr, "Failed to open file: %s\n", full_path);
            perror("Error");
            return 2;
        }

        fseek(contex->archive_file, (long)fe->offset, SEEK_SET);

        uint8_t  ciphertext[CIPHER_CHUNK];
        uint64_t bytes_remain = fe->size;

        while (bytes_remain > 0) {
            uint32_t to_read = (bytes_remain < sizeof(ciphertext))
                               ? (uint32_t)bytes_remain
                               : (uint32_t)sizeof(ciphertext);

            uint32_t bytes_read = fread(ciphertext, 1, to_read, contex->archive_file);
            if (bytes_read == 0) break;

            if (bytes_read < MAC_SIZE) {
                fprintf(stderr, "Truncated ciphertext block.\n");
                fclose(ofile);
                return 5;
            }

            uint8_t plaintext[CHUNK_SIZE];
            uint8_t block_nonce[NONCE_SIZE];
            derive_nonce(block_nonce, contex->nonce, contex->block_counter++);

            if (crypto_secretbox_open_easy(plaintext, ciphertext, bytes_read,
                                           block_nonce, contex->key) != 0) {
                fprintf(stderr, "Decryption failed for file: %s\n", fe->filename);
                fclose(ofile);
                return 10;
            }

            uint32_t plain_len = bytes_read - MAC_SIZE;
            fwrite(plaintext, 1, plain_len, ofile);
            bytes_remain -= bytes_read;
        }

        fclose(ofile);
    }
    return 0;
}

int load_archive(const char *archive_path, const char *mode, const char *pwd, archive_context_t *contex) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialize libsodium\n");
        return 8;
    }

    FILE *arch = fopen(archive_path, mode);
    if (!arch) {
        fprintf(stderr, "Failed to open archive: %s\n", archive_path);
        perror("Error");
        return 2;
    }

    contex->archive_file = arch;
    strcpy(contex->archive_path, archive_path);

    if (fread(contex->salt,  1, SALT_SIZE,  arch) != SALT_SIZE ||
        fread(contex->nonce, 1, NONCE_SIZE, arch) != NONCE_SIZE)
    {
        fprintf(stderr, "Archive too short (preamble).\n");
        fclose(arch);
        return 3;
    }

    // Reconstruct entry_nonce with the same tweak used during archiving
    memcpy(contex->entry_nonce, contex->nonce, NONCE_SIZE);
    contex->entry_nonce[NONCE_SIZE - 1] ^= 0xFF;
    contex->block_counter = 0;
    contex->entry_counter = 0;

    if (crypto_pwhash(
            contex->key, KEY_SIZE,
            pwd, strlen(pwd),
            contex->salt,
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_DEFAULT) != 0)
    {
        fprintf(stderr, "Key derivation failed (out of memory?).\n");
        fclose(arch);
        return 9;
    }

    archive_header_t header;
    if (fread(&header, sizeof(header), 1, arch) != 1) {
        fprintf(stderr, "Failed to read archive header.\n");
        fclose(arch);
        return 3;
    }

    if (memcmp(header.magic, MAGIC, 4) != 0) {
        fprintf(stderr, "Unknown magic in archive.\n");
        fclose(arch);
        return 6;
    }

    if (header.file_count == 0) {
        fprintf(stderr, "Archive contains no files.\n");
        fclose(arch);
        return 0;
    }

    contex->header = header;

    contex->entries = get_entries(arch, &contex->header, contex->key, contex->entry_nonce);
    if (!contex->entries) {
        fprintf(stderr, "Failed to get entries.\n");
        fclose(arch);
        return 7;
    }

    return 0;
}

int free_archive(archive_context_t *contex) {
    if (contex->entries)      free(contex->entries);
    if (contex->archive_file) fclose(contex->archive_file);
    return 0;
}

archive_file_entry_t *get_entries(FILE *archive_file, archive_header_t *header,
                                   const uint8_t key[KEY_SIZE],
                                   const uint8_t entry_nonce[NONCE_SIZE]) {
    archive_file_entry_t *entries = calloc(header->file_count, sizeof(*entries));
    if (!entries) {
        fprintf(stderr, "Out of memory.\n");
        return NULL;
    }

    size_t blob_size = sizeof(archive_file_entry_t) + MAC_SIZE;
    fseek(archive_file, header->file_table_offset, SEEK_SET);

    for (uint64_t i = 0; i < header->file_count; i++) {
        uint8_t blob[sizeof(archive_file_entry_t) + MAC_SIZE];

        if (fread(blob, 1, blob_size, archive_file) != blob_size) {
            fprintf(stderr, "Failed to read file table entry %llu.\n",
                    (unsigned long long)i);
            free(entries);
            return NULL;
        }

        uint8_t nonce[NONCE_SIZE];
        derive_nonce(nonce, entry_nonce, i);

        if (crypto_secretbox_open_easy((uint8_t *)&entries[i], blob, blob_size,
                                        nonce, key) != 0) {
            fprintf(stderr, "Failed to decrypt entry %llu (wrong password?).\n",
                    (unsigned long long)i);
            free(entries);
            return NULL;
        }
    }

    return entries;
}

archive_file_entry_t *get_file_entry_by_name(archive_file_entry_t *entries,
                                              const char *name, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        if (strcmp(entries[i].filename, name) == 0)
            return &entries[i];
    }
    return NULL;
}

archive_file_entry_t *get_file_entry_by_id(archive_file_entry_t *entries,
                                             uint64_t id, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        if (entries[i].id == id)
            return &entries[i];
    }
    return NULL;
}

int init_header(archive_header_t *header) {
    header->magic[0] = 'T';
    header->magic[1] = 'E';
    header->magic[2] = 'T';
    header->magic[3] = 'O';
    header->version           = ARCHIVE_VER;
    header->file_count        = 0;
    header->file_table_offset = 0;
    header->data_table_offset = 0;
    return 0;
}

void create_directory_recursive(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof(tmp), "%s", path);

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            MKDIR(tmp, 0755);
            *p = '/';
        }
    }
    MKDIR(tmp, 0755);
}