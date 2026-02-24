#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdbool.h>
#include <stdlib.h>

/**
 * Dynamic array of strings
 */
typedef struct {
    char **data;
    size_t len;
    size_t capacity;  // Track capacity for better memory management
} StringArray;

/**
 * Parsed command-line arguments
 */
typedef struct {
    StringArray flags;      // Arguments starting with '-' or '--'
    StringArray arguments;  // Positional arguments
} Args;

/**
 * Parse command-line arguments into flags and positional arguments
 * @param argv StringArray containing command-line arguments (including program name)
 * @return Pointer to Args structure, or NULL on allocation failure
 */
Args *argparse_parse(StringArray *argv);

/**
 * Free all memory associated with an Args structure
 * @param args Args structure to free
 */
void argparse_free(Args *args);

/**
 * Check if a flag exists in the parsed arguments
 * @param args Parsed arguments
 * @param flag Flag to search for (e.g., "-v" or "--verbose")
 * @return true if flag exists, false otherwise
 */
bool argparse_has_flag(const Args *args, const char *flag);

/**
 * Get the value of a flag (the argument following the flag)
 * @param args Parsed arguments
 * @param flag Flag to search for
 * @param argv Original argv to find the next value
 * @return Pointer to the value string, or NULL if not found
 */
const char *argparse_get_flag_value(const Args *args, const char *flag, const StringArray *argv);

/**
 * Search for a value in a StringArray
 * @param str_arr StringArray to search
 * @param val Value to find
 * @return Index of first match, or -1 if not found
 */
int string_array_includes(const StringArray *str_arr, const char *val);

/**
 * Search for a substring within a string
 * @param str String to search in
 * @param expr Substring to search for
 * @return Pointer to first occurrence, or NULL if not found
 */
char *str_includes(const char *str, const char *expr);

/**
 * Check if a string starts with a given prefix
 * @param str String to check
 * @param prefix Prefix to search for
 * @return true if str starts with prefix, false otherwise
 */
bool str_startswith(const char *str, const char *prefix);

/**
 * Compare two strings for equality
 * @param s1 First string
 * @param s2 Second string
 * @return true if strings are equal, false otherwise
 */
bool str_equals(const char *s1, const char *s2);

/**
 * Get the length of a string
 * @param s String to measure
 * @return Length of string (excluding null terminator)
 */
size_t str_length(const char *s);

#endif /* ARGPARSE_H */