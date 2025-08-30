#ifndef CORE_PATH_H
#define CORE_PATH_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <uuid/uuid.h>

#define DATA "data"
#define META "__meta"
#define META_TABLES "tables.meta"
#define META_COLUMNS "columns.meta"
#define META_TABLE_FILE "table.meta"

int
path_data(char* out_result, size_t buf_size);

int
path_db(const char* db_name, char* out_result, size_t buf_size);

int
path_db_meta(const char* db_name, char* out_result, size_t buf_size);

int
path_db_meta_tables(const char* db_name, char* out_result, size_t buf_size);

int
path_db_meta_columns(const char* db_name, char* out_result, size_t buf_size);

int
path_db_table(const char* db_name, const char* table_name, char* out_result, size_t buf_size);

int
path_db_table_meta(const char* db_name, const char* table_name, char* out_result, size_t buf_size);
int
path_db_table_data(const char* db_name, const char* table_name, char* out_result, size_t buf_size);

int
path_db_table_page(
    const char* db_name, const char* table_name, uuid_t uuid, char* out_result, size_t buf_size);

#endif