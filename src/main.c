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
        if (argparse_has_flag(args, "--pwd") && argparse_has_flag(args, "--dest-dir")) {
            const char *src_dir  = argparse_get_flag_value(args, "--pack", argv_array);
            const char *dest_arch = argparse_get_flag_value(args, "--dest-dir", argv_array);
            const char *pwd = argparse_get_flag_value(args, "--pwd", argv_array);

            return create_archive(dest_arch, src_dir, pwd);
        }

        printf("No password provided. Exiting.\n");
        return 1;
    } else if (argparse_has_flag(args, "--unpack")) {
        const char *arch_path = argparse_get_flag_value(args, "--unpack", argv_array);
        const char *dst_dir = argparse_get_flag_value(args, "--dest-dir", argv_array);
        const char *pwd = argparse_get_flag_value(args, "--pwd", argv_array);

        return extract_archive(arch_path, dst_dir, pwd);
    } else if (argparse_has_flag(args, "--get-entries")) {
        const char *path = argparse_get_flag_value(args, "--get-entries", argv_array);
        const char *pwd = argparse_get_flag_value(args, "--pwd", argv_array);

        printf("Reading archive: %s\n", path);

        archive_context_t contex;
        if (load_archive(path, "rb", pwd, &contex) != 0) {
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
    printf("  --pack <src_dir>               Create a new archive from a directory.\n");
    printf("  --unpack <archive_path>        Extract an existing archive to a directory.\n");
    printf("  --dest-dir <dest_dir>          The destination directory.\n");
    printf("  --pwd <password>               The password for the archive.\n");
    printf("  --get-entries <archive_path>   List all files and directories within an archive.\n");
    printf("  --version, -v                  Display version information and exit.\n");
    printf("  --help, -h                     Display this help message and exit.\n");
    printf("\nExample:\n");
    printf("  %s --pack ./my_files backup.sec\n", TOOL_NAME);
}