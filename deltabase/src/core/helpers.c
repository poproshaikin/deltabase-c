#include "include/data_token.h"
#include <stdlib.h>
#include <string.h>

DataToken *make_token(DataType type, const void *data, size_t size) {
    DataToken *token = malloc(sizeof(DataToken));
    token->type = type;
    token->size = size;
    token->bytes = malloc(size);
    memcpy(token->bytes, data, size);
    return token;
}

void free_token(DataToken *token) {
    free(token->bytes);
    free(token);
}
