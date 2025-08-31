#include "include/meta.h"

int
find_column(const char* name, const MetaTable* table, MetaColumn *out) {
    if (!out) {
        return 1;
    }

    for (size_t i = 0; i < table->columns_count; i++) {
        if (strcmp(table->columns[i].name, name) == 0) {
            *out = table->columns[i];
            return 0;
        }
    }

    return 2;
}

ssize_t
get_table_column_index(const uuid_t column_id, const MetaTable* table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        if (uuid_compare(table->columns[i].id, column_id) == 0) {
            return i;
        }
    }
    return -1;
}