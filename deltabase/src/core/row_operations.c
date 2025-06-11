#include "include/core.h"
#include "include/data_io.h"
#include "include/data_page.h"
#include "include/data_table.h"
#include "include/data_token.h"
#include "include/path_service.h"
#include "include/utils.h"
#include <linux/limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int write_row_update_state(const DataRow *row, MetaTable *schema, FILE *file) {
    int fd = fileno(file);

    PageHeader header;
    read_page_header(file, &header);
    if (header.file_size >= MAX_PAGE_SIZE) {
        return 1;
    }

    header.rows_count++;
    header.file_size += dr_size(schema, row);
    header.max_rid = ++schema->last_rid;

    lseek(fd, 0, SEEK_SET);
    if (write_ph(&header, fd) != 0) {
        return 2;
    }

    lseek(fd, 0, SEEK_END);
    if (write_dr(schema, row, fd) != 0) {
        return 3;
    }

    return 0;
}

int insert_row(const char *db_name, const char *table_name, const DataRow *row) {
    char buffer[PATH_MAX];  
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);

    MetaTable schema;
    if (get_table_schema(db_name, table_name, &schema) != 0) {
        return 1;
    }
    
    size_t pages_count;
    char **pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }

    for (size_t i = 0; i < pages_count; i++) {
        FILE *file = fopen(pages[i], "r+");
        if (!file) {
            fprintf(stderr, "Failed to open page for reading\n");
            return i + 3; // as the number of iteration it failed on + ensure that collision with two previous error codes wouldn't happen
        }

        int result = write_row_update_state(row, &schema, file);
        if (result == 1) { // means that the page is out of space
            continue;
        } else if (result > 1) { // something went wrong
            return i + 3; 
        } 

        fclose(file);
        break;
    }

    for (size_t i = 0; i < pages_count; i++) {
        free(pages[i]);
    }
    free(pages);

    return 0;
}

bool perform_filter_operation(const void *value1, const DataType type1, const void *value2, const DataType type2, const FilterOp operation) {
    if (type1 == DT_NULL || type2 == DT_NULL) {
        return false; 
    }

    if (type1 != type2) {
        // TODO: implicit casting for compatible types (e.g. INT to REAL etc)
        return false;
    }
    
    switch(type1) {
        case DT_INTEGER: {
            int32_t a = *(const int32_t *)value1;
            int32_t b = *(const int32_t *)value2;

            switch (operation) {
                case OP_EQ:  return a == b;
                case OP_NEQ: return a != b;
                case OP_LT:  return a < b;
                case OP_LTE: return a <= b;
                case OP_GT:  return a > b;
                case OP_GTE: return a >= b;
            }
            break;
        }
        case DT_REAL: {
            double a = *(const double *)value1;
            double b = *(const double *)value2;
            switch (operation) {
                case OP_EQ:  return a == b;
                case OP_NEQ: return a != b;
                case OP_LT:  return a < b;
                case OP_LTE: return a <= b;
                case OP_GT:  return a > b;
                case OP_GTE: return a >= b;
            }
            break;
        }
        case DT_STRING: {
            const char *a = (const char *)value1;
            const char *b = (const char *)value2;
            int cmp = strcmp(a, b);
            switch (operation) {
                case OP_EQ:  return cmp == 0;
                case OP_NEQ: return cmp != 0;
                case OP_LT:  return cmp < 0;
                case OP_LTE: return cmp <= 0;
                case OP_GT:  return cmp > 0;
                case OP_GTE: return cmp >= 0;
            }
            break;
        }
        case DT_CHAR: {
            char a = *(const char *)value1;
            char b = *(const char *)value2;
            switch (operation) {
                case OP_EQ:  return a == b;
                case OP_NEQ: return a != b;
                case OP_LT:  return a < b;
                case OP_LTE: return a <= b;
                case OP_GT:  return a > b;
                case OP_GTE: return a >= b;
            }
            break;
        }
        case DT_BOOL: {
            bool a = *(const bool *)value1;
            bool b = *(const bool *)value2;
            switch (operation) {
                case OP_EQ:  return a == b;
                case OP_NEQ: return a != b;
                default:     return false;
            }
        }
    }

    fprintf(stderr, "Unsupported comparison: %i to %i\n", type1, type2);

    return false;
}

bool row_satisfies_filter(const MetaTable *schema, const DataRow *row, const DataFilter *filter) {
    if (!filter->is_node) {
        ssize_t col_index = get_column_index_meta(filter->data.condition.column_id, schema);

        if (col_index == -1) { // wtf
            return false;
        }

        if (row->count < col_index) { // wtf
            return false;
        }

        if (filter->data.condition.)
    }
}

int update_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, const DataRow *new_row) {
    char buffer[PATH_MAX];
    path_db_table_meta(db_name, table_name, buffer, PATH_MAX);

    MetaTable schema;
    if (get_table_schema(db_name, table_name, &schema) != 0) {
        return 1;
    }
    
    size_t pages_count;
    char **pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }
    
    for (size_t i = 0; i < pages_count; i++) {
        FILE *file = fopen(pages[i], "r+");
        if (!file) {
            fprintf(stderr, "Failed to open page for reading\n");
            return i + 3; // as the number of iteration it failed on + ensure that collision with two previous error codes wouldn't happen
        }
        int fd = fileno(file);

        PageHeader header;
        if (read_ph(&header, fd) != 0) {
            return i + 3;
        }

        for (size_t j = 0; j < header.rows_count; j++) {
            DataRow row;
            if (read_dr(&schema, &row, fd) != 0) {

            }
        }
    }
}
