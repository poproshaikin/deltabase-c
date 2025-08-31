#include "include/core.h"
#include "include/paths.h"
#include "include/utils.h"
#include "include/ll_io.h"
#include <stdio.h>

int 
ensure_fs_initialized() {
    char buffer[PATH_MAX];

    if (path_data(buffer, PATH_MAX) != 0) {
        fprintf(stderr, "In ensure_fs_initialized: path_data returned error\n");
        return 1;
    }

    if (!dir_exists(buffer)) {
        mkdir_recursive(buffer, 0755);
    }

    return 0;
}

int
create_database(const char* db_name) {
    char buffer[PATH_MAX];
    if (path_db(db_name, buffer, PATH_MAX) != 0) {
        fprintf(stderr,"In create_database: failed to get database path\n");
        return 1;
    }

    if (dir_exists(buffer)) {
        fprintf(stderr, "In create_database: database already exists\n");
        return 2;
    }

    if (mkdir_recursive(buffer, 0755) != 0) {
        fprintf(stderr, "in create_database: failed to make directories recursively\n");
        return 3;
    }

    if (path_db_meta(db_name, buffer, PATH_MAX) != 0) {
        fprintf(stderr, "In create_database: failed to get database meta path\n");
        return 4;
    }

    if (mkdir_recursive(buffer, 0755) != 0) {
        fprintf(stderr, "in create_database: failed to make directories recursively\n");
        return 3;
    }

    return 0;
}

int
drop_database(const char* db_name) {
    char db_path[PATH_MAX];
    path_db(db_name, db_path, sizeof(db_path));

    if (!dir_exists(db_path)) {
        fprintf(stderr, "Database '%s' doesn't exists\n", db_name);
        return 1;
    }

    int result = rmdir_recursive(db_path);

    if (result == 0) {
        printf("Database '%s' dropped successfully.\n", db_name);
        return 0;
    } else {
        fprintf(stderr, "Failed to drop database '%s'.\n", db_name);
        return 2;
    }
}

bool
exists_database(const char* db_name) {
    char db_path[PATH_MAX];
    if (path_db(db_name, db_path, sizeof(db_path)) != 0) {
        fprintf(stderr, "In exists_database: to get database path\n");
        return false;
    }

    return dir_exists(db_path);
}

int
create_table(const char* db_name, const MetaTable* table) {
    int op = 0;

    char buffer[PATH_MAX];
    path_db_table(db_name, table->name, buffer, PATH_MAX);
    if (dir_exists(buffer)) {
        fprintf(stderr, "Table '%s' already exists \n", table->name);
        return 1;
    }

    if (mkdir_recursive(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", table->name);
        return 2;
    }

    path_db_table_meta(db_name, table->name, buffer, PATH_MAX);

    FILE* meta_file = fopen(buffer, "w+");
    if (!meta_file) {
        return 3;
    }

    if ((op = write_mt(table, fileno(meta_file))) != 0) {
        printf("error: create_table:write_mt %i\n", op);
        fclose(meta_file);
        return 4;
    }
    fclose(meta_file);

    path_db_table_data(db_name, table->name, buffer, PATH_MAX);
    if (mkdir_recursive(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", table->name);
        return 5;
    }

    return 0;
}

bool
exists_table(const char* db_name, const char* table_name) {
    char table_path[PATH_MAX];
    path_db_table(db_name, table_name, table_path, sizeof(table_path));

    return dir_exists(table_path);
}

int
get_table(const char* db_name, const char* table_name, MetaTable* out) {
    if (!exists_table(db_name, table_name)) {
        return 1;
    }

    int res = 0;
    char buffer[PATH_MAX];

    path_db_table_meta(db_name, table_name, buffer, PATH_MAX);

    FILE* file = fopen(buffer, "r");
    int fd = fileno(file);

    if ((res = read_mt(out, fd)) != 0) {
        fclose(file);
        return res;
    }

    fclose(file);
    return 0;
}

int
update_table(const char* db_name, const MetaTable* table) {
    char buffer[PATH_MAX];
    path_db_table_meta(db_name, table->name, buffer, PATH_MAX);

    FILE* file = fopen(buffer, "w+");
    int fd = fileno(file);

    if (write_mt(table, fd) != 0) {
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

static int
write_row_update_state(const char* db_name, DataRow* row, MetaTable* schema, FILE* file) {
    int fd = fileno(file);

    PageHeader header;
    read_ph(&header, fd);
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

    if (update_table(db_name, schema) != 0) {
        return 4;
    }

    return 0;
}

int
insert_row(const char* db_name, MetaTable* table, DataRow* row) {
    char buffer[PATH_MAX];
    path_db_table_data(db_name, table->name, buffer, PATH_MAX);

    size_t pages_count = 0;
    char** pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }

    if (pages_count == 0) {
        char* new_page_path = NULL;
        if (create_page(db_name, table->name, NULL, &new_page_path) != 0) {
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
        FILE* file = fopen(pages[i], "r+");
        if (!file) {
            fprintf(stderr, "Failed to open page for reading\n");
            return i + 4; // as the number of iteration it failed on + ensure that
                          // collision with two previous error codes wouldn't happen
        }

        int result = write_row_update_state(db_name, row, table, file);
        if (result == 1) { // means that the page is out of space
            char* ptr;
            if (create_page(db_name, table->name, NULL, &ptr) != 0) {
                fprintf(stderr, "Failed to create new page while insertion\n");
                return -1 - i;
            }

            pages = realloc(pages, (pages_count + 1) * sizeof(char*));
            if (!pages) {
                for (size_t i = 0; i < pages_count; i++) {
                    free(pages[i]);
                }
                free(pages);
                fprintf(stderr, "Failed to allocate memory\n");
                return -1 - i;
            }

            pages[pages_count++] = ptr;

            fclose(file);
            continue;
        }
        
        if (result > 1) { // something went wrong
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

#define ROW_CALLBACK_PARAMS                                                                        \
    const char *db_name, MetaTable *schema, PageHeader *header, int fd, size_t row_pos,            \
        uint64_t rid, const DataRow *row, const void *user_data

typedef int (*RowCallback)(ROW_CALLBACK_PARAMS);

static int
for_each_row_in_page(const char* db_name,
                     const char* page_path,
                     MetaTable* table,
                     const DataFilter* filter,
                     size_t* rows_affected,
                     RowCallback callback,
                     const void* user_data) {
    FILE* file = fopen(page_path, "r+");
    if (!file) {
        fprintf(stderr, "Failed to open page for reading\n");
        return 1;
    }
    int fd = fileno(file);

    PageHeader header;
    if (read_ph(&header, fd) != 0) {
        return 2;
    }

    int old_rows_count = header.rows_count;
    for (size_t i = 0; i < old_rows_count; i++) {
        DataRow row;

        size_t row_start_pos = lseek(fd, 0, SEEK_CUR);

        if (read_dr(table, NULL, 0, &row, fd) != 0) {
            return -1 - i; // just some unique error code
        }

        if (row.flags & RF_OBSOLETE) {
            continue;
        }

        if (!row_satisfies_filter(table, &row, filter)) {
            continue;
        }

        // save position before callback
        ssize_t pos = lseek(fd, 0, SEEK_CUR);

        int result =
            callback(db_name, table, &header, fd, row_start_pos, row.row_id, &row, user_data);

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
        return 3;
    }

    fclose(file);
    return 0;
}

// Implementation moved to future/threading/ for later integration
// typedef struct ForEachRowWorkerArg { ... };
// static void *for_each_row_thread_worker(void *arg) { ... };
// static int for_each_row_matching_filter_multithreaded(...) { ... };

static int
for_each_row_matching_filter_impl(const char* db_name,
                                  const char* table_name,
                                  const DataFilter* filter,
                                  RowCallback callback,
                                  const void* user_data,
                                  size_t* rows_affected) {
    MetaTable schema;
    if (get_table(db_name, table_name, &schema) != 0) {
        return 1;
    }

    char buffer[PATH_MAX];
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);

    size_t pages_count;
    char** pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }

    for (size_t i = 0; i < pages_count; i++) {
        if (for_each_row_in_page(
                db_name, pages[i], &schema, filter, rows_affected, callback, user_data) != 0) {
            fprintf(stderr, "Failed to process page %s\n", pages[i]);
            for (size_t j = 0; j < i; j++) {
                free(pages[j]);
            }
            free(pages);
            return 3;
        }
    }

    for (size_t i = 0; i < pages_count; i++) {
        free(pages[i]);
    }

    return 0;
}

int
for_each_row_matching_filter(const char* db_name,
                             const char* table_name,
                             const DataFilter* filter,
                             RowCallback callback,
                             void* user_data,
                             size_t* rows_affected) {
    if (!db_name || !table_name || !filter || !callback) {
        fprintf(stderr, "Invalid parameters in for_each_row_matching_filter\n");
        return 2;
    }

    // Use single-threaded implementation for now
    // TODO: Add multithreading support in the future
    return for_each_row_matching_filter_impl(
        db_name, table_name, filter, callback, user_data, rows_affected);
}

static int
combine_row(DataRow* out, MetaTable* schema, const DataRow* old_row, const DataRowUpdate* update) {
    out->tokens = malloc(schema->columns_count * sizeof(DataToken));
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
        if (j < update->count &&
            uuid_compare(schema->columns[i].id, update->column_indices[j]) == 0) {
            // count size
            size_t size = dtype_size(schema->columns[i].data_type);

            if (size == 0) {
                switch (schema->columns[i].data_type) {
                case DT_STRING:
                    size = strlen(update->values[j]);
                    break;
                default:
                    return 2;
                }
            }

            // create a token
            out->tokens[i] = make_token(schema->columns[i].data_type, update->values[j], size);

            j++;
        } else {
            // leave an old token
            out->tokens[i] = copy_token(old_row->tokens[i]);
        }
    }

    return 0;
}

int
update_callback(ROW_CALLBACK_PARAMS) {
    lseek(fd, row_pos, SEEK_SET);
    lseek(fd, sizeof(uint64_t), SEEK_CUR); // length
    lseek(fd, sizeof(row->row_id), SEEK_CUR);
    if (write_dr_flags(row->flags | RF_OBSOLETE, fd) != 0) {
        return 1;
    }

    lseek(fd, 0, SEEK_END);

    DataRow combined;
    if (combine_row(&combined, schema, row, (DataRowUpdate*)user_data) != 0) {
        fprintf(stderr, "Failed to combine row while updating\n");
        free_tokens(combined.tokens, combined.count);
        return 2;
    }

    if (write_dr(schema, &combined, fd) != 0) {
        return 3;
    }

    header->rows_count++;

    // need to save because of incrementing the last_rid in combine_row
    update_table(db_name, schema);
    return 0;
}

int
update_rows_by_filter(const char* db_name,
                      const char* table_name,
                      const DataFilter* filter,
                      const DataRowUpdate* update,
                      size_t* rows_affected) {
    return for_each_row_matching_filter(
        db_name, table_name, filter, update_callback, (void*)update, rows_affected);
}

int
delete_callback(ROW_CALLBACK_PARAMS) {
    lseek(fd, row_pos, SEEK_SET);
    lseek(fd, sizeof(uint64_t), SEEK_CUR); // size
    lseek(fd, sizeof(uint64_t), SEEK_CUR); // id

    if (write_dr_flags(row->flags | RF_OBSOLETE, fd) != 0) {
        return 1;
    }

    return 0;
}

int
delete_rows_by_filter(const char* db_name,
                      const char* table_name,
                      const DataFilter* filter,
                      size_t* rows_affected) {
    return for_each_row_matching_filter(
        db_name, table_name, filter, delete_callback, NULL, rows_affected);
}

int
seq_scan(const char* db_name,
         const char* table_name,
         const char** column_names,
         size_t columns_count,
         const DataFilter* filter,
         DataTable* out) {
    char buffer[PATH_MAX];
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);

    MetaTable* schema = malloc(sizeof(MetaTable));
    if (!schema) {
        fprintf(stderr, "Failed to allocate memory for meta table while full scan\n");
        return 1;
    }

    if (get_table(db_name, table_name, schema) != 0) {
        return 2;
    }

    size_t pages_count;
    char** pages = get_dir_files(buffer, &pages_count);
    if (!pages)
        return 3;

    // create an array of pointers to pointers to datarow corresponding for each
    // page
    DataRow** page_rows = calloc(pages_count, sizeof(DataRow*));
    size_t* rows_per_page = calloc(pages_count, sizeof(size_t));
    size_t total_rows = 0;

    for (size_t i = 0; i < pages_count; i++) {
        FILE* file = fopen(pages[i], "r");
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
        DataRow* page = calloc(header.rows_count, sizeof(DataRow));
        if (!page) {
            fprintf(stderr, "Failed to allocate memory in full_scan\n");
            goto fail_cleanup;
        }
        for (size_t j = 0; j < header.rows_count; j++) {
            DataRow row;

            int res = 0;
            if ((res = read_dr(schema, column_names, columns_count, &row, fd)) != 0) {
                fprintf(stderr, "Failed to read data row in full_scan: %i\n", res);
                fclose(file);
                goto fail_cleanup;
            }

            if (row.flags & RF_OBSOLETE) {
                free_row(&row);
                continue;
            }

            if (filter && !row_satisfies_filter(schema, &row, filter)) {
                free_row(&row);
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
    DataRow* all_rows = malloc(total_rows * sizeof(DataRow));
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
        if (!page_rows[i])
            continue;
        for (size_t j = 0; j < rows_per_page[i]; j++) {
            free_row(&page_rows[i][j]);
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

char**
get_databases(size_t* out_count) {
    char buffer[PATH_MAX];

    if (path_data(buffer, PATH_MAX) != 0) {
        fprintf(stderr, "In get_databases: path_data returned error\n");
        return NULL;
    }

    return get_directory_names(buffer, out_count);
}

char**
get_tables(const char* db_name, size_t* out_count) { 
    char buffer[PATH_MAX];

    if (path_db(db_name, buffer, PATH_MAX) != 0) {
        fprintf(stderr, "In get_tables: path_db returned error\n");
        return NULL;
    }

    size_t dirs_count = 0;
    char **dirs = get_directory_names(buffer, &dirs_count);
    if (!dirs) {
        fprintf(stderr, "In get_tables: get_directory_names returned NULL\n");
        return NULL;
    }

    bool meta_exists = false;
    for (size_t i = 0; i < dirs_count; i++) {
        if (strcmp(dirs[i], META) == 0) {
            meta_exists = true;
            break;
        }
    }

    if (!meta_exists) {
        if (out_count) {
            *out_count = dirs_count;
        }
        return dirs;
    }

    if (dirs_count == 1) {
        free(dirs[0]);
        free(dirs);

        if (out_count) *out_count = 0;
        return NULL;
    }

    char **without_meta = malloc((dirs_count - 1) * sizeof(char*));
    if (!without_meta) {
        fprintf(stderr, "In get_tables: failed to allocate memory for without_meta (%lu bytes)\n", (dirs_count - 1) * sizeof(char*));
        return NULL;
    }

    size_t counter = 0;
    for (size_t i = 0; i < dirs_count; i++) {
        if (strcmp(dirs[i], META) == 0) {
            free(dirs[i]);
            continue;
        }

        without_meta[counter++] = dirs[i];
    }

    free(dirs);
    if (out_count) {
        *out_count = counter;
    }

    return without_meta;
}
