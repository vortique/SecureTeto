#include "archiver.h"
#include "metadata.h"
#include "argparse.h"

#include <stdio.h>

int handle_arguments(Args *args, StringArray *argv_array);
void print_help();

int main(int argc, char **argv) {
    StringArray argv_array = {
        .data = argv,
        .len = (size_t)argc,
        .capacity = (size_t)argc
    };

    Args *parsed_args = argparse_parse(&argv_array);
    if (!parsed_args) {
        fprintf(stderr, "%s: Failed to parse arguments\n", TOOL_NAME);
        return 1;
    }

    int result = handle_arguments(parsed_args, &argv_array);

    return result;
}

int handle_arguments(Args *args, StringArray *argv_array) {
    if (argparse_has_flag(args, "--version") || argparse_has_flag(args, "-v")) {
        printf("%s %d.%d.%d\n", TOOL_NAME, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
        return 0;
    }
    
    if (argparse_has_flag(args, "--help") || argparse_has_flag(args, "-h") || args->arguments.len == 0) {
        print_help();
        return 0;
    }

    if (argparse_has_flag(args, "--pack")) {
        const char *src_dir  = args->arguments.data[0];
        const char *dst_arch = args->arguments.data[1];

        return create_archive(dst_arch, src_dir);
    } else if (argparse_has_flag(args, "--unpack")) {
        const char *arch_path = args->arguments.data[0];
        const char *dst_dir = args->arguments.data[1];

        return extract_archive(arch_path, dst_dir);
    } else if (argparse_has_flag(args, "--get-entries")) {
        const char *path = argparse_get_flag_value(args, "--get-entries", argv_array);

        printf("Reading archive: %s\n", path);

        archive_context_t contex;
        if (load_archive(path, "rb", &contex) != 0) {
            printf("Error while loading archive.\n");
            return 1;
        }

        for (uint64_t i = 0; i < contex.header.file_count; i++) {
            printf("%s: (id: %llu, offset: %llu, size: %llu, type: %s)\n", contex.entries[i].filename, contex.entries[i].id,
                contex.entries[i].offset, contex.entries[i].size, contex.entries[i].type ? "dir" : "file");
        }

        if (free_archive(&contex) != 0) {
            printf("Error while freeing archive.\n");
            return 1;
        }

        return 0;
    }

    return 0;
}

void print_help() {
    printf("Usage: %s [OPTIONS]\n\n", TOOL_NAME);
    printf("Options:\n");
    printf("  --pack <src_dir> <dst_arch>    Create a new archive from a directory.\n");
    printf("  --unpack <arch_path> <dst_dir> Extract an existing archive to a directory.\n");
    printf("  --get-entries <arch_path>      List all files and directories within an archive.\n");
    printf("  --help, -h                     Display this help message and exit.\n");
    printf("\nExample:\n");
    printf("  %s --pack ./my_files backup.sec\n", TOOL_NAME);
}