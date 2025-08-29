//
// Created by poproshaikin on 8.7.25.
//

#ifndef CORE_H
#define CORE_H

#include "data.h"
#include "misc.h"
#include <linux/limits.h>
#include <stdbool.h>

int 
ensure_fs_initialized();

int
create_database(const char* db_name);

int
drop_database(const char* db_name);

bool
exists_database(const char* db_name);

char** 
get_databases(size_t* out_count);


int
create_table(const char* db_name, const MetaTable* table);

bool
exists_table(const char* db_name, const char* table_name);

int
get_table(const char* db_name, const char* table_name, MetaTable* out);

int
update_table(const char* db_name, const MetaTable* table);

char**
get_tables(const char *db_name, size_t *out_count);


int
insert_row(const char* db_name, const char* table_name, DataRow* row);

int
update_rows_by_filter(const char* db_name,
                      const char* table_name,
                      const DataFilter* filter,
                      const DataRowUpdate* update,
                      size_t* rows_affected);

int
delete_rows_by_filter(const char* db_name,
                      const char* table_name,
                      const DataFilter* filter,
                      size_t* rows_affected);


int
seq_scan(const char* db_name,
         const char* table_name,
         const char** column_names,
         size_t columns_count,
         const DataFilter* filter,
         DataTable* out);

#endif // CORE_H
