#ifndef CORE_H
#define CORE_H

#include "data_filter.h"
#include "data_table.h"
#include "data_page.h"

int create_database(const char *name);
int drop_database(const char *name);
bool exists_database(const char *db_name);

int create_table(const char *db_name, const MetaTable *scheme);
int drop_table(const char *db_name, const char *table_name);
bool exists_table(const char *db_name, const char *table_name);

int insert_row(const char *db_name, const char *table_name, DataRow *row);

typedef struct {
    size_t count;
    uuid_t *column_indices;
    void **values;
} DataRowUpdate;
 
int update_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, const DataRowUpdate *update, size_t *rows_affected);
int delete_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, size_t *rows_affected);

int full_scan(const char *db_name, const char *table_name, const char **column_names, size_t columns_count, DataTable *out);

int get_table_schema(const char *db_name, const char *table_name, MetaTable *out);
int list_tables(char ***out_table_names, size_t *count);
int save_table_schema(const char *db_name, const MetaTable *meta);

int create_page(const char *db_name, const char *table_name, PageHeader *out_new_page, char **out_path);
ssize_t get_pages(const char *db_name, const char *table_name, char ***out_paths);
int read_page_header(FILE *file, PageHeader *out);

#endif
