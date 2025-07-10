#ifndef CORE_PATH_H
#define CORE_PATH_H

#include <stddef.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include <stdio.h>

void path_db(const char *db_name, char *out_result, uint64_t buf_size);

void path_db_meta(const char *db_name, char *out_result, uint64_t buf_size);
void path_db_meta_tables(const char *db_name, char *out_result, uint64_t buf_size);
void path_db_meta_columns(const char *db_name, char *out_result, uint64_t buf_size);

void path_db_table(const char *db_name, const char *table_name, char *out_result, uint64_t buf_size);
void path_db_table_meta(const char *db_name, const char *table_name, char *out_result, uint64_t buf_size);
void path_db_table_data(const char *db_name, const char *table_name, char *out_result, uint64_t buf_size);

void path_db_table_page(const char *db_name, const char *table_name, uuid_t uuid, char *out_result, uint64_t buf_size);

#endif