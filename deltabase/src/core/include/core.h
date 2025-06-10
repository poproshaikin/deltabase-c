#ifndef CORE_H
#define CORE_H

#include "data_filter.h"
#include "data_table.h"
#include "data_page.h"

int create_database(const char *name);
int drop_database(const char *name);

int create_table(const char *db_name, const char *table_name, const DataSchema *scheme);
int drop_table(const char *db_name, const char *table_name);

int insert_row(const char *table_name, const DataRow *row);
int update_row(const char *table_name, size_t row_id, const DataRow *new_row);
int delete_row(const char *table_name, size_t row_id);

int full_scan(char *table_name, DataTable *out);
int select_rows(const char *table_name, const DataFilter *filter, DataRow *out);
int get_row(const char *table_name, size_t row_id, DataRow *out);
int write_row(const char *table_name, DataRow row);

int get_table_schema(const char *table_name, DataSchema *out);
int list_tables(char ***table_names, size_t *count);

int create_page(char *table_name, PageHeader *out_new_page);
int get_pages  (char *table_name, char **out_names);

#endif
