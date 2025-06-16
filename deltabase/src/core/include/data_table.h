#ifndef DATA_TABLE_H
#define DATA_TABLE_H

#include "data_token.h"
#include <string.h>
#include <stdint.h>
#include <uuid/uuid.h>
#include <stdbool.h>

typedef enum {
    CF_NONE = 0,
    CF_PK = 1 << 0,
    CF_FK = 1 << 1,
    CF_AI = 1 << 2,
    CF_NN = 1 << 3,
    CF_UN = 1 << 4
} DataColumnFlags;

/* Column description */
typedef struct {
    uuid_t column_id;
    char *name;

    DataType data_type;
    DataColumnFlags flags;
} MetaColumn;

void free_col(MetaColumn *column);

/* Table scheme */
typedef struct {
    uuid_t table_id;
    char *name;

    bool has_pk;
    uuid_t pk;
    
    MetaColumn **columns;
    uint64_t columns_count;

    uint64_t last_rid;
} MetaTable;

typedef enum {
    RF_NONE = 0,
    RF_OBSOLETE = 1 << 0,
} DataRowFlags;

/* Data row */
typedef struct {
    uint64_t row_id;
    DataRowFlags flags;
    DataToken **tokens;
    uint64_t count;
} DataRow;

typedef struct {
    DataRow **rows;
    size_t count;
} DataRowSet;

void free_tokens(DataToken **tokens, size_t count);
void free_row(DataRow *row);
void free_meta_table(MetaTable *table);

uint64_t dr_size(const MetaTable *schema, const DataRow *row);
uint64_t dr_size_v(const MetaTable *schema, const DataToken **tokens, int tokens_count);

// gets index of the column COLUMN_ID in the TABLE
ssize_t get_column_index_meta(const uuid_t column_id, const MetaTable *table);

inline MetaColumn *find_column(const char *name, const MetaTable *table) {
    for (size_t i = 0; i < table->columns_count; i++) {
        if (strcmp(table->columns[i]->name, name) == 0) {
            return table->columns[i];
        }
    }

    return NULL;
}

typedef struct {
    char *name;
    int64_t last;
    int64_t increment;
} DataSequence;

typedef struct {
    MetaTable *scheme;
    DataRow **rows;
    size_t rows_count;
} DataTable;

#endif
