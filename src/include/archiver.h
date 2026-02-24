#ifndef ARCHIVER_H
#define ARCHIVER_H

#include <stdint.h>
#include <stdio.h>

#define MAX_FILENAME_LENGTH 512

#pragma pack(push, 1)
typedef struct {
    char     magic[4]; // "SECU"
    uint32_t version;
    uint64_t file_count;
    uint64_t file_table_offset;
    uint64_t data_table_offset;
} archive_header_t;

typedef struct {
    char filename[MAX_FILENAME_LENGTH];
    uint64_t offset; // is 0 if dir
    uint64_t size;   // is 0 if dir
} archive_file_entry_t;
#pragma pack(pop)

/**
 * @brief Creates an archive from the specified directory.
 * @param archive_path The path to the archive file to create.
 * @param dir_path The path to the directory to archive.
 * @return 0 on success, error code otherwise.
 */
int create_archive(const char *archive_path, const char *dir_path);

/**
 * @brief Recursively writes file entries and data for a directory.
 * @param header Pointer to the archive header.
 * @param archive_file The archive file pointer.
 * @param entry_pos Current position in the file table.
 * @param path The current directory path.
 * @param rel_path The relative path.
 * @return 0 on success, error code otherwise.
 */
int write_and_create_entry_dir(archive_header_t *header, FILE *archive_file, uint64_t entry_pos, const char *path, const char *rel_path);

/**
 * @brief Scans the directory and counts the number of entries.
 * @param path The directory path to scan.
 * @return The number of entries.
 */
uint64_t scan_entry_count(const char *path);

/**
 * @brief Initializes the archive header with default values.
 * @param header Pointer to the header to initialize.
 * @return 0 on success.
 */
int init_header(archive_header_t *header);

#endif // ARCHIVER_H