#include "argparse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void string_array_init(StringArray *arr) {
    arr->data = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

static bool string_array_append(StringArray *arr, char *str) {
    if (arr->len >= arr->capacity) {
        size_t new_capacity = arr->capacity == 0 ? 8 : arr->capacity * 2;
        char **tmp = realloc(arr->data, new_capacity * sizeof(char *));
        if (!tmp) {
            return false;
        }
        arr->data = tmp;
        arr->capacity = new_capacity;
    }
    arr->data[arr->len] = str;
    arr->len++;
    return true;
}

static void string_array_cleanup(StringArray *arr) {
    if (arr && arr->data) {
        free(arr->data);
        arr->data = NULL;
        arr->len = 0;
        arr->capacity = 0;
    }
}

Args *argparse_parse(StringArray *args) {
    if (!args || !args->data || args->len == 0) {
        Args *empty = malloc(sizeof(Args));
        if (!empty) {
            return NULL;
        }
        string_array_init(&empty->flags);
        string_array_init(&empty->arguments);
        return empty;
    }

    Args *parsed_args = malloc(sizeof(Args));
    if (!parsed_args) {
        return NULL;
    }

    string_array_init(&parsed_args->flags);
    string_array_init(&parsed_args->arguments);

    /* Skip program name (args->data[0]) */
    for (size_t i = 1; i < args->len; i++) {
        char *current = args->data[i];

        if (str_startswith(current, "-")) {
            if (!string_array_append(&parsed_args->flags, current)) {
                argparse_free(parsed_args);
                return NULL;
            }
        } else {
            if (!string_array_append(&parsed_args->arguments, current)) {
                argparse_free(parsed_args);
                return NULL;
            }
        }
    }

    return parsed_args;
}

void argparse_free(Args *args) {
    if (!args) {
        return;
    }
    string_array_cleanup(&args->flags);
    string_array_cleanup(&args->arguments);
    free(args);
}

bool argparse_has_flag(const Args *args, const char *flag) {
    if (!args || !flag) {
        return false;
    }
    return string_array_includes(&args->flags, flag) != -1;
}

const char *argparse_get_flag_value(const Args *args, const char *flag, const StringArray *argv) {
    if (!args || !flag || !argv) {
        return NULL;
    }

    /* Find the flag in the original argv */
    for (size_t i = 1; i < argv->len; i++) {
        if (str_equals(argv->data[i], flag)) {
            /* Check if there's a next argument and it's not a flag */
            if (i + 1 < argv->len && !str_startswith(argv->data[i + 1], "-")) {
                return argv->data[i + 1];
            }
            return NULL;
        }
    }
    return NULL;
}

int string_array_includes(const StringArray *str_arr, const char *val) {
    if (!str_arr || !val) {
        return -1;
    }

    for (size_t i = 0; i < str_arr->len; i++) {
        if (str_equals(str_arr->data[i], val)) {
            return (int)i;
        }
    }
    return -1;
}

char *str_includes(const char *str, const char *expr) {
    if (!str || !expr) {
        return NULL;
    }

    if (*expr == '\0') {
        return (char *)str;
    }

    size_t expr_len = str_length(expr);
    const char *current = str;

    while (*current != '\0') {
        if (str_length(current) < expr_len) {
            break;
        }

        const char *s = current;
        const char *e = expr;
        bool match = true;

        while (*e != '\0') {
            if (*s != *e) {
                match = false;
                break;
            }
            s++;
            e++;
        }

        if (match) {
            return (char *)current;
        }

        current++;
    }

    return NULL;
}

bool str_startswith(const char *str, const char *prefix) {
    if (!str || !prefix) {
        return false;
    }

    if (*prefix == '\0') {
        return true;
    }

    if (*str == '\0') {
        return false;
    }

    if (str_length(str) < str_length(prefix)) {
        return false;
    }

    while (*prefix != '\0') {
        if (*str != *prefix) {
            return false;
        }
        str++;
        prefix++;
    }

    return true;
}

bool str_equals(const char *s1, const char *s2) {
    if (s1 == NULL && s2 == NULL) {
        return true;
    }
    if (s1 == NULL || s2 == NULL) {
        return false;
    }

    while (*s1 != '\0' && *s2 != '\0') {
        if (*s1 != *s2) {
            return false;
        }
        s1++;
        s2++;
    }

    return (*s1 == *s2);
}

size_t str_length(const char *s) {
    if (!s) {
        return 0;
    }

    const char *p = s;
    while (*p != '\0') {
        p++;
    }
    return (size_t)(p - s);
}

/* Legacy function name for backward compatibility */
Args *parser(StringArray *args) {
    return argparse_parse(args);
}

/* Legacy function name for backward compatibility */
bool str_comp(const char *s1, const char *s2) {
    return str_equals(s1, s2);
}