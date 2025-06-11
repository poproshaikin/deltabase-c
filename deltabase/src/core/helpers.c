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

void free_token(DataToken *token) {
    free(token->bytes);
    free(token);
}

ssize_t get_column_index_meta(const uuid_t column_id, const MetaTable *table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        if (uuid_compare(table->columns[i]->column_id, column_id)) {
            return i;
        }
    }
    return -1;
}
