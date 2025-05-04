#include "string-utils.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char** str_split(char* a_str, const char a_delim, size_t *out_len)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = (char**)malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    *out_len = count;
    return result;
}

char *str_substring(const char *str, int start, int length) {
    if (start < 0 || length < 0 || start >= strlen(str)) {
        return NULL;
    }

    char* sub = malloc(length + 1);

    strncpy(sub, str + start, length);

    sub[length] = '\0';

    return sub;
}

bool str_contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

bool char_isnumber(const char c) {
    return (c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-' || c == 'e' || c == 'E';
}

bool str_isnumber(const char *str) {
    if (!str || !*str) return false;

    int dotCount = 0;

    for (int i = 0; str[i]; i++) {
        if (str[i] == '.') {
            dotCount++;
            if (dotCount > 1) return false;
        } else if (str[i] < '0' || str[i] > '9') {
            return false;
        }
    }

    return true;
}

bool str_isint(const char *str) {
    if (str == NULL || *str == '\0') {
        return false;
    }

    if (*str == '+' || *str == '-') {
        str++;
    }

    if (*str == '\0') {
        return false;
    }

    while (*str) {
        if (!isdigit(*str)) {
            return false;  
        }
        str++;
    }

    return true;  // Если строка состоит только из цифр (с учётом возможного знака), это целое число
}
