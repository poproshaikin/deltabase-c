#include "include/core.h"
#include "include/data_filter.h"
#include "include/data_io.h"
#include "include/data_page.h"
#include "include/data_table.h"
#include "include/data_token.h"
#include "include/path_service.h"
#include "include/utils.h"
#include <bits/types/cookie_io_functions_t.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

int create_page(const char *db_name, const char *table_name, PageHeader *out_new_page, char **out_path) {
    uuid_generate_time(out_new_page->page_id);
    out_new_page->rows_count = 0;

    char file_path[PATH_MAX];
    path_db_table_page(db_name, table_name, out_new_page->page_id, file_path, PATH_MAX);

    FILE *file = fopen(file_path, "w+");
    if (!file) {
        fprintf(stderr, "Failed to create page file %s\n", out_new_page->page_id);
        return 1;
    }

    if (write_ph(out_new_page, fileno(file)) != 0) {
        return 2;
    }

    size_t len = strlen(file_path);
    char *path = calloc(len + 1, sizeof(char)); 
    if (!path) {
        fclose(file);
        return 3;
    }

    memcpy(path, file_path, len); 
    path[len] = '\0'; 
    *out_path = path; 

    fclose(file);
    return 0;
}

// if success, returns count of paths. otherwise, -1
ssize_t get_pages(const char *db_name, const char *table_name, char ***out_paths) {
    if (!out_paths) {
        fprintf(stderr, "get_pages: out_paths cannot be null");
        return -1;
    }
    char dir_path[PATH_MAX];
    path_db_table_data(db_name, table_name, dir_path, PATH_MAX);

    size_t files_count = 0;
    *out_paths = get_dir_files(dir_path, &files_count);

    return files_count;
}

static int write_row_update_state(const char *db_name, DataRow *row, MetaTable *schema, FILE *file) {
    int fd = fileno(file);

    PageHeader header;
    read_page_header(file, &header);
    if (header.file_size >= MAX_PAGE_SIZE) {
        return 1;
    }

    header.rows_count++;
    header.file_size += dr_size(schema, row);
    header.max_rid = schema->last_rid;
    row->row_id = schema->last_rid;
    schema->last_rid++;

    lseek(fd, 0, SEEK_SET);
    if (write_ph(&header, fd) != 0) {
        return 2;
    }

    lseek(fd, 0, SEEK_END);
    if (write_dr(schema, row, fd) != 0) {
        return 3;
    }

    if (save_table_schema(db_name, schema) != 0) {
        return 4;
    }

    return 0;
}

int insert_row(const char *db_name, const char *table_name, DataRow *row) {
    char buffer[PATH_MAX];  
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);

    MetaTable schema;
    if (get_table_schema(db_name, table_name, &schema) != 0) {
        return 1;
    }
    
    size_t pages_count = 0;
    char **pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }

    if (pages_count == 0) {
        PageHeader header;
        char *new_page_path = NULL;
        if (create_page(db_name, table_name, &header, &new_page_path) != 0) {
            return 3;
        }

        pages = malloc(sizeof(char*));
        if (!pages) {
            free(new_page_path);
            return 4;
        }

        pages[0] = new_page_path;
        pages_count = 1;
    }

    for (size_t i = 0; i < pages_count; i++) {
        FILE *file = fopen(pages[i], "r+");
        if (!file) {
            fprintf(stderr, "Failed to open page for reading\n");
            return i + 4; // as the number of iteration it failed on + ensure that collision with two previous error codes wouldn't happen
        }

        int result = write_row_update_state(db_name, row, &schema, file);
        if (result == 1) { // means that the page is out of space
            fclose(file);
            continue;
        } else if (result > 1) { // something went wrong
            fclose(file);
            return i + 4; 
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

bool apply_filter(DataFilterCondition condition, const void *db_value, const DataType db_type) {
    char col_id[37];
    
    if (condition.type == DT_NULL || db_type == DT_NULL) {
        return false; 
    }

    if (condition.type != db_type) {
        fprintf(stderr, "apply_filter: type in condition isn't the same as type in database\n");
        // TODO: implicit casting for compatible types (e.g. INT to REAL etc)
        return false;
    }

    switch(condition.type) {
        case DT_INTEGER: {
            int32_t a = *(const int32_t *)condition.value;
            int32_t b = *(const int32_t *)db_value;
            
            switch (condition.op) {
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
            double a = *(const double *)condition.value;
            double b = *(const double *)db_value;
            switch (condition.op) {
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
            const char *a = (const char *)condition.value;
            const char *b = (const char *)db_value;
            
            int cmp = strcmp(a, b);
            switch (condition.op) {
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
            char a = *(const char *)condition.value;
            char b = *(const char *)db_value;
            switch (condition.op) {
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
            bool a = *(const bool *)condition.value;
            bool b = *(const bool *)db_value;
            switch (condition.op) {
                case OP_EQ:  return a == b;
                case OP_NEQ: return a != b;
                default:     return false;
            }
        }
        case DT_NULL:
          break;
        }

    fprintf(stderr, "Unsupported comparison: %i to %i\n", condition.type, db_type);

    return false;
}


bool row_satisfies_filter(const MetaTable *schema, const DataRow *row, const DataFilter *filter) {
    if (schema->columns_count != row->count) {
        fprintf(stderr, "row_satisfies_filter: Count of columns in schema wasn't equal to count of tokens in row\n");
        return false;
    }

    if (!filter->is_node) {
        ssize_t col_index = get_column_index_meta(filter->data.condition.column_id, schema);

        if (col_index < 0 || col_index >= row->count) { // wtf
            fprintf(stderr, "Column with ID provided in filter was not found in table\n");
            return false;
        }

        return apply_filter(
            filter->data.condition,
            row->tokens[col_index]->bytes, 
            row->tokens[col_index]->type
        );
    }
    else {
        switch (filter->data.node.op) {
            case LOGIC_AND:
                return 
                    row_satisfies_filter(schema, row, filter->data.node.left) &&
                    row_satisfies_filter(schema, row, filter->data.node.right);
            case LOGIC_OR:
                return 
                    row_satisfies_filter(schema, row, filter->data.node.left) ||
                    row_satisfies_filter(schema, row, filter->data.node.right);
            default:
                fprintf(stderr, "Unsupported logic operator: %i\n", filter->data.node.op);
                return false;
        }
    }
}

static int combine_row(DataRow *out, MetaTable *schema, const DataRow *old_row, const DataRowUpdate *update) {
    out->tokens = malloc(schema->columns_count * sizeof(DataToken *));
    if (!out->tokens) {
        fprintf(stderr, "Failed to allocate memory in combine_row\n");
        return 1;
    }
    out->count = old_row->count;
    out->flags = old_row->flags;
    out->row_id = ++schema->last_rid;
    
    size_t j = 0;
    for (size_t i = 0; i < schema->columns_count; i++) {
        // if column indices are equal
        if (j < update->count && uuid_compare(schema->columns[i]->column_id, update->column_indices[j]) == 0) {
            // count size
            size_t size = dtype_size(schema->columns[i]->data_type);

            if (size == 0) {
                switch (schema->columns[i]->data_type) {
                    case DT_STRING: 
                        size = strlen(update->values[j]);
                        break;
                    default: return 2;
                }
            }

            // create a token
            out->tokens[i] = make_token(schema->columns[i]->data_type, update->values[j], size);

            j++;
        } else {
            // leave an old token
            out->tokens[i] = copy_token(old_row->tokens[i]);
        }
    }

    return 0;
}

#define ROW_CALLBACK_PARAMS const char *db_name, const char *table_name, MetaTable *schema, PageHeader *header, int fd, size_t row_pos, uint64_t rid, const DataRow *row, const void *user_data 

typedef int (*RowCallback)(ROW_CALLBACK_PARAMS);

int for_each_row(const char *db_name, const char *table_name, RowCallback callback, const void *user_data) {  
    return 0;
}

int for_each_row_matching_filter(const char *db_name, const char *table_name, const DataFilter *filter, RowCallback callback, const void *user_data, size_t *rows_affected) {
    MetaTable schema;
    if (get_table_schema(db_name, table_name, &schema) != 0) {
        return 1;
    }

    char buffer[PATH_MAX];
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);
    
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
            return i + 4;
        }

        int old_rows_count = header.rows_count;
        for (size_t j = 0; j < old_rows_count; j++) {
            DataRow row;

            size_t row_start_pos = lseek(fd, 0, SEEK_CUR);

            if (read_dr(&schema, NULL, 0, &row, fd) != 0) {
                return -1 - j; // just some unique error code
            }

            if (row.flags & RF_OBSOLETE) {
                continue;
            }
            
            if (!row_satisfies_filter(&schema, &row, filter)) {
                continue;
            }

            // save position before callback
            ssize_t pos = lseek(fd, 0, SEEK_CUR);

            const DataRowUpdate *update = user_data;

            int result = callback(db_name, table_name, &schema, &header, fd, row_start_pos, row.row_id, &row, user_data);
            if (result != 0) {
                fclose(file);
                return result;
            }
            
            // restore position
            lseek(fd, pos, SEEK_SET);
            if (rows_affected)
                (*rows_affected)++;
        }   

        lseek(fd, 0, SEEK_SET);
        if (write_ph(&header, fd) != 0) {
            return i + 5;
        }

        fclose(file);
    }

    for (size_t i = 0; i < pages_count; i++) {
        free(pages[i]);
    }

    return 0;
}

int update_callback(ROW_CALLBACK_PARAMS) {
    lseek(fd, row_pos, SEEK_SET);
    lseek(fd, sizeof(uint64_t), SEEK_CUR); // length
    lseek(fd, sizeof(row->row_id), SEEK_CUR);
    if (write_dr_flags(row->flags | RF_OBSOLETE, fd) != 0) { 
        return 1;   
    }

    lseek(fd, 0, SEEK_END);

    DataRow combined;
    if (combine_row(&combined, schema, row, (DataRowUpdate *)user_data) != 0) {
        fprintf(stderr, "Failed to combine row while updating\n");
        free_tokens(combined.tokens, combined.count);
        return 2;
    }

    if (write_dr(schema, &combined, fd) != 0) {
        return 3;
    }
    
    header->rows_count++; 
    
    // need to save because of increasing last_rid in combine_row
    save_table_schema(db_name, schema);
    return 0;
}

int update_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, const DataRowUpdate *update, size_t *rows_affected) {
    return for_each_row_matching_filter(db_name, table_name, filter, update_callback, update, rows_affected);
}

int delete_callback(ROW_CALLBACK_PARAMS) {
    lseek(fd, row_pos, SEEK_SET);
    lseek(fd, sizeof(uint64_t), SEEK_CUR); // size
    lseek(fd, sizeof(uint64_t), SEEK_CUR); // id

    if (write_dr_flags(row->flags | RF_OBSOLETE, fd) != 0) { 
        return 1;   
    }

    return 0;
}

int delete_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, size_t *rows_affected) {
    return for_each_row_matching_filter(db_name, table_name, filter, delete_callback, NULL, rows_affected);   
}

int full_scan(const char *db_name, const char *table_name, const char **column_names, size_t columns_count, const DataFilter *filter, DataTable *out) {
    char buffer[PATH_MAX];
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);

    MetaTable *schema = malloc(sizeof(MetaTable));
    if (!schema) {
        fprintf(stderr, "Failed to allocate memory for meta table while full scan\n");
        return 1;
    }

    if (get_table_schema(db_name, table_name, schema) != 0) {
        return 2;
    }

    size_t pages_count;
    char **pages = get_dir_files(buffer, &pages_count);
    if (!pages) return 3;

    // create an array of pointers to pointers to datarow corresponding for each page 
    DataRow ***page_rows = calloc(pages_count, sizeof(DataRow **));
    size_t *rows_per_page = calloc(pages_count, sizeof(size_t));
    size_t total_rows = 0;

    for (size_t i = 0; i < pages_count; i++) {
        FILE *file = fopen(pages[i], "r");
        if (!file) {
            fprintf(stderr, "Failed to open file %s\n", pages[i]);
            goto fail_cleanup;
        }

        int fd = fileno(file);
        PageHeader header;
        if (read_ph(&header, fd) != 0) {
            fclose(file);
            goto fail_cleanup;
        }

        size_t count = 0;
        DataRow **page = calloc(header.rows_count, sizeof(DataRow *));
        if (!page) {
            fprintf(stderr, "Failed to allocate memory in full_scan\n");
            goto fail_cleanup;
        }
        for (size_t j = 0; j < header.rows_count; j++) {
            DataRow *row = malloc(sizeof(DataRow));
            if (!row) {
                fprintf(stderr, "Failed to allocate memory in full_scan\n");
                fclose(file);
                goto fail_cleanup;
            }

            int res = 0;
            if ((res = read_dr(schema, column_names, columns_count, row, fd)) != 0) {
                fprintf(stderr, "Failed to read data row in full_scan: %i\n", res);
                free(row);
                fclose(file);
                goto fail_cleanup;
            }

            if (row->flags & RF_OBSOLETE) {
                free_row(row);
                continue;
            }

            if (filter && !row_satisfies_filter(schema, row, filter)) {
                free_row(row);
                continue;
            }

            page[count++] = row;
        }

        fclose(file);
        page_rows[i] = page;
        rows_per_page[i] = count;
        total_rows += count;
    }

    // unite all rows in the one array of pointers 
    DataRow **all_rows = calloc(total_rows, sizeof(DataRow *));
    size_t pos = 0;
    for (size_t i = 0; i < pages_count; i++) {
        for (size_t j = 0; j < rows_per_page[i]; j++) {
            all_rows[pos++] = page_rows[i][j];
        }
        free(page_rows[i]);
    }

    out->scheme = schema;
    out->rows = all_rows;
    out->rows_count = total_rows;

    free(page_rows);
    free(rows_per_page);

    for (size_t i = 0; i < pages_count; i++) {
        free(pages[i]);
    }

    return 0;

fail_cleanup:
    for (size_t i = 0; i < pages_count; i++) {
        if (!page_rows[i]) continue;
        for (size_t j = 0; j < rows_per_page[i]; j++) {
            free_row(page_rows[i][j]);
        }
        free(page_rows[i]);
    }
    free(page_rows);
    free(rows_per_page);

    for (size_t i = 0; i < pages_count; i++) {
        free(pages[i]);
    }

    return 4;
}

/*
int update_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, const DataRowUpdate *update) {
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
                return -1 - j; // just some unique error code
            }
            
            if (!row_satisfies_filter(&schema, &row, filter)) {
                continue;
            }
    
            size_t rowsize = dr_size(&schema, &row);
            if (write_dr_flags(row.flags | RF_OBSOLETE, fd) != 0) { 
                return -1 - j;   
            }

            lseek(fd, 0, SEEK_END);

            DataRow combined;
            if (combine_row(&combined, &schema, &row, update) != 0) {
                fprintf(stderr, "Failed to combine row while updating\n");
                free_tokens(combined.tokens, combined.count);
                return i + 3;
            }

            if (write_dr(&schema, &combined, fd) != 0) {
                return i + 3;
            }

            fclose(file);
            break;
        }
    }

    return 0;
}
*/
