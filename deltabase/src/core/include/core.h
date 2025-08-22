//
// Created by poproshaikin on 8.7.25.
//

#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "misc.h"
#include "utils.h"
#include "paths.h"
#include "ll_io.h"

#include <linux/limits.h>
#include <stdbool.h>

typedef struct CoreContext {
    // Future: Add threading support
    // ThreadPool *thread_pool;
    bool placeholder; // Temporary field to keep struct valid
} CoreContext;

int create_database(const char *db_name, const CoreContext *ctx);
int drop_database(const char *db_name, const CoreContext *ctx);
bool exists_database(const char *db_name, const CoreContext *ctx);

int create_table(const char *db_name, const MetaTable *table, const CoreContext *ctx);
bool exists_table(const char *db_name, const char *table_name, const CoreContext *ctx);
int get_table(const char *db_name, const char *table_name, MetaTable *out, const CoreContext *ctx);
int update_table(const char *db_name, const MetaTable *table, const CoreContext *ctx);

int insert_row(const char *db_name, const char *table_name, DataRow *row, const CoreContext *ctx);

int update_rows_by_filter(
    const char *db_name, 
    const char *table_name, 
    const DataFilter *filter, 
    const DataRowUpdate *update, 
    size_t *rows_affected, 
    const CoreContext *ctx
);

int delete_rows_by_filter(
    const char *db_name, 
    const char *table_name, 
    const DataFilter *filter, 
    size_t *rows_affected, 
    const CoreContext *ctx
);

int seq_scan(
    const char *db_name, 
    const char *table_name, 
    const char **column_names, 
    size_t columns_count, 
    const DataFilter *filter, 
    DataTable *out, 
    const CoreContext *ctx
);

int index_scan(const char *db_name, const char *index_name, const DataFilter *filter, DataTable *out, const CoreContext *ctx);

#endif //CORE_H
