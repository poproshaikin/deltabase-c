#ifndef DATA_TOKEN_H
#define DATA_TOKEN_H

#include <stddef.h>
#include <stdio.h>

/* Data types */
typedef enum {
    DT_NULL = -1, 
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,
    DT_BOOL,
    DT_STRING,
} DataType;

typedef struct DataToken {
    size_t size; // Used if the TYPE has dynamic size.
    char *bytes;
    DataType type;
} DataToken;

DataToken *make_token(DataType type, const void *data, size_t size);
void free_token(DataToken *token);

#endif
