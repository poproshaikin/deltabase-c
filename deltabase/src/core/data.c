#include "include/data.h"

DataToken *make_token(DataType type, const void *data, size_t size) {
    DataToken *token = malloc(sizeof(DataToken));
    token->type = type;
    token->size = size;
    token->bytes = (char *)malloc(size);
    memcpy(token->bytes, data, size);
    return token;
}

void free_token(DataToken *token) {
    if (token) {
        free(token->bytes);
        free(token);
    }
}

DataToken *copy_token(const DataToken *old) {
    if (!old) return NULL;
    DataToken *new_token = malloc(sizeof(DataToken));
    new_token->type = old->type;
    new_token->size = old->size;
    new_token->bytes = malloc(old->size);
    memcpy(new_token->bytes, old->bytes, old->size);
    return new_token;
}

void free_tokens(DataToken **tokens, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free_token(tokens[i]);
    }
}

void free_row(DataRow *row) {
    free_tokens(row->tokens, row->count);
    free(row);
}

void free_col(MetaColumn *column) {
    free(column->name);
    free(column);
}

