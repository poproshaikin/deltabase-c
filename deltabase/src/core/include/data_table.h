#ifndef DATA_TABLE_H
#define DATA_TABLE_H

#include "data_token.h"
#include <stdint.h>
#include <uuid/uuid.h>
#include <stdbool.h>

typedef enum {
    CF_PK = 0b00001,
    CF_FK = 0b00010,
    CF_AI = 0b00100,
    CF_NN = 0b01000,
    CF_UN = 0b10000
} DataColumnFlags;

/* Column description */
typedef struct {
    uuid_t column_id;
    char *name;

    DataType data_type;
    DataColumnFlags flags;
} MetaColumn;

/* Table scheme */
typedef struct {
    uuid_t table_id;
    char *name;

    bool has_pk;
    uuid_t pk;
    
    MetaColumn **columns;
    size_t columns_count;

    uint64_t last_rid;
} MetaTable;

typedef enum {
    DT_OBSOLETE = 0b1,
} DataRowFlags;

/* Data row */
typedef struct {
    uint64_t row_id;
    DataRowFlags flags;
    DataToken **tokens;
    size_t count;
} DataRow;

uint64_t dr_size(const MetaTable *schema, const DataRow *row);
uint64_t dr_size_v(const MetaTable *schema, const DataToken **tokens, int tokens_count);

// gets index of the column COLUMN_ID in the TABLE
ssize_t get_column_index_meta(const uuid_t column_id, const MetaTable *table);

typedef struct {
    MetaTable *scheme;
    DataRow **rows;
} DataTable;

#endif
