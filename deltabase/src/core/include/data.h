#ifndef CORE_DATA_H
#define CORE_DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "meta.h"

typedef struct DataToken {
    size_t size; // Used if the TYPE has dynamic size.
    char* bytes;
    DataType type;
} DataToken;

DataToken*
make_token(DataType type, const void* data, size_t size);

DataToken*
copy_token(const DataToken* old);

void
free_token(DataToken* token);

void
free_tokens(DataToken** tokens, size_t count);

/* Data row */
typedef struct {
    uint64_t row_id;
    DataRowFlags flags;
    DataToken** tokens;
    uint64_t count;
} DataRow;

uint64_t
dr_size(const MetaTable* schema, const DataRow* row);

uint64_t
dr_size_v(const MetaTable* schema, const DataToken** tokens, int tokens_count);

void
free_row(DataRow* row);

typedef struct {
    DataRow** rows;
    size_t count;
} DataRowSet;

typedef struct {
    MetaTable* scheme;
    DataRow** rows;
    size_t rows_count;
} DataTable;

typedef enum {
    OP_EQ = 1, // ==
    OP_NEQ,    // !=
    OP_LT,     // <
    OP_LTE,    // <=
    OP_GT,     // >
    OP_GTE     // >=
} FilterOp;

typedef enum { LOGIC_AND = 1, LOGIC_OR } LogicOp;

typedef struct DataFilter DataFilter;

typedef struct {
    uuid_t column_id;
    FilterOp op;
    DataType type;
    void* value;
} DataFilterCondition;

typedef struct {
    DataFilter* left;
    LogicOp op;
    DataFilter* right;
} DataFilterNode;

struct DataFilter {
    bool is_node;
    union {
        DataFilterCondition condition;
        DataFilterNode node;
    } data;
};

bool
row_satisfies_filter(const MetaTable* schema, const DataRow* row, const DataFilter* filter);

DataFilter*
create_filter_condition(uuid_t column_id, FilterOp op, DataType type, const void* value);

DataFilter*
create_filter_node(DataFilter* left, LogicOp op, DataFilter* right);

void
free_filter(DataFilter* filter);

int
create_page(const char* db_name, const char* table_name, PageHeader* out_new_page, char** out_path);

ssize_t
get_pages(const char* db_name, const char* table_name, char*** out_paths);

#endif // CORE_DATA_H