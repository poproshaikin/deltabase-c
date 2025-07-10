#include "include/meta.h"

MetaColumn *find_column(const char *name, const MetaTable *table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        if (strcmp(table->columns[i]->name, name) == 0) {
            return table->columns[i];
        }
    }

    return NULL;
}

ssize_t get_table_column_index(const uuid_t column_id, const MetaTable *table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        if (uuid_compare(table->columns[i]->column_id, column_id) == 0) {
            return i;
        }
    }
    return -1;
}