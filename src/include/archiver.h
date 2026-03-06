#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <stdint.h>
#include <stdio.h>
#include <sodium.h>

#define MAX_FILENAME_LENGTH 512

#define NONCE_SIZE crypto_secretbox_NONCEBYTES
#define SALT_SIZE  crypto_pwhash_SALTBYTES
#define KEY_SIZE   crypto_secretbox_KEYBYTES
#define MAC_SIZE   crypto_secretbox_MACBYTES

#define ARCHIVE_VER 4

#pragma pack(push, 1)
typedef struct
{
    char magic[4];
    uint32_t version;
    uint64_t file_count;
    uint64_t file_table_offset;
    uint64_t data_table_offset;
} archive_header_t;

typedef struct
{
    char filename[MAX_FILENAME_LENGTH];
    uint64_t id;
    uint64_t offset;
    uint64_t size;
    uint8_t type;
} archive_file_entry_t;

typedef struct
{
    char archive_path[MAX_FILENAME_LENGTH * 2];
    uint8_t salt[SALT_SIZE];
    uint8_t nonce[NONCE_SIZE];
    uint8_t entry_nonce[NONCE_SIZE]; // separate namespace for entry table
    uint8_t key[KEY_SIZE];
    uint64_t block_counter;
    uint64_t entry_counter;
    FILE *archive_file;
    archive_header_t header;
    archive_file_entry_t *entries;
} archive_context_t;

typedef struct
{
    uint8_t key[KEY_SIZE];
    uint8_t nonce[NONCE_SIZE];
    uint8_t entry_nonce[NONCE_SIZE]; // separate namespace for entry table
    uint64_t block_counter;
    uint64_t entry_counter;
} sodium_context_t;
#pragma pack(pop)

int create_archive(const char *archive_path, const char *dir_path, const char *pwd);

int write_and_create_entry_dir(archive_header_t *header,
                               FILE *archive_file,
                               uint64_t *entry_pos,
                               const char *path,
                               const char *rel_path,
                               sodium_context_t *sodium_ctx);

uint64_t scan_entry_count(const char *path);

int init_header(archive_header_t *header);

int extract_archive(const char *archive_path, const char *dst_dir, const char *pwd);

int extract_entry(archive_context_t *contex, const char *dst_dir, const char *pwd);

int create_dirs(archive_context_t *contex, const char *dst_dir, const char *pwd);

int write_files(archive_context_t *contex, const char *dst_dir);

int load_archive(const char *archive_path, const char *mode, const char *pwd, archive_context_t *contex);

int free_archive(archive_context_t *contex);

archive_file_entry_t *get_entries(FILE *archive_file, archive_header_t *header,
                                   const uint8_t key[KEY_SIZE],
                                   const uint8_t entry_nonce[NONCE_SIZE]);

archive_file_entry_t *get_file_entry_by_name(archive_file_entry_t *entries, const char *name, uint64_t count);

archive_file_entry_t *get_file_entry_by_id(archive_file_entry_t *entries, uint64_t id, uint64_t count);

void create_directory_recursive(const char *path);

#endif // ARCHIVER_H