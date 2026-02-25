#include "archiver.h"
#include "metadata.h"
#include "argparse.h"

#include <stdio.h>

int handle_arguments(Args *args);

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

    int result = handle_arguments(parsed_args);

    return result;
}

int handle_arguments(Args *args) {
    if (argparse_has_flag(args, "--pack")) {
        const char *src_dir  = args->arguments.data[0];
        const char *dst_arch = args->arguments.data[1];

        return create_archive(dst_arch, src_dir);
    } else if (argparse_has_flag(args, "--unpack")) {
        const char *arch_path = args->arguments.data[0];
        const char *dst_dir = args->arguments.data[1];

        return extract_archive(arch_path, dst_dir);
    }

    return 0;
}