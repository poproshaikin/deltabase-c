#ifndef DATA_TOKEN_H
#define DATA_TOKEN_H

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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

static inline DataToken *make_token(DataType type, const void *data, size_t size) {
    DataToken *token = (DataToken *)malloc(sizeof(DataToken));
    token->type = type;
    token->size = size;
    token->bytes = (char *)malloc(size);
    memcpy(token->bytes, data, size);
    return token;
}

DataToken *copy_token(const DataToken *old);
void free_token(DataToken *token);

#endif
