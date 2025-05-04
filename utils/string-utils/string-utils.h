#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>
#include <stdbool.h>

char **str_split(char* a_str, const char a_delim, size_t *out_len);

char *str_substring(const char *str, int start, int length);

bool str_contains(const char *haystack, const char *needle);

bool char_isnumber(const char c);
bool str_isnumber(const char *str);
bool str_isint(const char *str);

#endif
