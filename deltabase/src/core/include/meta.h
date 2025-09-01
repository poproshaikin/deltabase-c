#ifndef CORE_META_H
#define CORE_META_H

#include <stdbool.h>
#include <stdint.h>
#include <uuid/uuid.h>

typedef enum {
    CF_NONE = 0,
    CF_PK = 1 << 0,
    CF_FK = 1 << 1,
    CF_AI = 1 << 2,
    CF_NN = 1 << 3,
    CF_UN = 1 << 4
} DataColumnFlags;

/* Data types */
typedef enum {
    DT_NULL = -1,
    DT_UNDEFINED = 0,
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,
    DT_BOOL,
    DT_STRING,
} DataType;

/* Column description */
typedef struct {
    uuid_t id;
    uuid_t table_id;
    char* name;

    DataType data_type;
    DataColumnFlags flags;
} MetaColumn;

void
free_col(MetaColumn* column);

/* Schema description */
typedef struct {
    uuid_t id;
    char* name;
} MetaSchema;

void
free_schema(MetaSchema* schema);

/* Table scheme */
typedef struct {
    uuid_t id;
    uuid_t schema_id;
    char* name;

    bool has_pk;
    MetaColumn* pk;

    MetaColumn* columns;
    uint64_t columns_count;

    uint64_t last_rid;
} MetaTable;

typedef enum {
    RF_NONE = 0,
    RF_OBSOLETE = 1 << 0,
} DataRowFlags;

// gets index of the column COLUMN_ID in the TABLE
ssize_t
get_table_column_index(const uuid_t column_id, const MetaTable* table);

int
find_column(const char* name, const MetaTable* table, MetaColumn* out);

#define MAX_PAGE_SIZE (8 * 1024)

/* Page header info */
typedef struct {
    uuid_t page_id;
    uint64_t rows_count;
    size_t file_size;
    uint64_t min_rid;
    uint64_t max_rid;
} PageHeader;

#endif // CORE_META_H