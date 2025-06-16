#include "include/data_table.h"
#include "include/data_token.h"

#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

DataToken *make_token(DataType type, const void *data, size_t size) {
    DataToken *token = malloc(sizeof(DataToken));
    token->type = type;
    token->size = size;
    token->bytes = malloc(size);
    memcpy(token->bytes, data, size);
    return token;
}

DataToken *copy_token(const DataToken *old) {
    if (!old) return NULL;

    DataToken *copy = malloc(sizeof(DataToken));
    if (!copy) return NULL;

    copy->type = old->type;
    copy->size = old->size;

    if (copy->size > 0) {
        copy->bytes = malloc(copy->size);
        if (!copy->bytes) {
            free(copy);
            return NULL;
        }
        memcpy(copy->bytes, old->bytes, copy->size);
    } else {
        copy->bytes = NULL;
    }

    return copy;
}

void free_token(DataToken *token) {
    free(token->bytes);
    free(token);
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

void free_meta_table(MetaTable *table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        free_col(table->columns[i]);
    }
    free(table->name);
    free(table);
}

ssize_t get_column_index_meta(const uuid_t column_id, const MetaTable *table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        if (uuid_compare(table->columns[i]->column_id, column_id)) {
            return i;
        }
    }
    return -1;
}

