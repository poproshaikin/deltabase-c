#include "include/path_service.h"

#include <stdio.h>

static const char *DATA = "data";
static const char *META = "__meta";
static const char *META_TABLES = "tables.meta";
static const char *META_COLUMNS = "columns.meta";
static const char *META_TABLE_FILE = "table.meta";

void path_db(const char *db_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s", DATA, db_name);
}

void path_db_meta(const char *db_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s/%s", DATA, db_name, META);
}

void path_db_meta_tables(const char *db_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s/%s/%s", DATA, db_name, META, META_TABLES);
}

void path_db_meta_columns(const char *db_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s/%s/%s", DATA, db_name, META, META_COLUMNS);
}

void path_db_table(const char *db_name, const char *table_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s", db_name, table_name);
}

void path_db_table_meta(const char *db_name, const char *table_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s/%s", db_name, table_name, META_TABLE_FILE);
}

void path_db_table_data(const char *db_name, const char *table_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s/%s/%s", DATA, db_name, table_name, DATA);
}

void path_db_table_page(const char *db_name, const char *table_name, uuid_t uuid, char *out_result, uint64_t buf_size) {
    char uuid_string[37];
    uuid_unparse_lower(uuid, uuid_string);
    snprintf(out_result, buf_size, "%s/%s/%s/%s/%s", DATA, db_name, table_name, DATA, uuid_string);
}
