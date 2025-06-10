#ifndef DATA_TABLE_H
#define DATA_TABLE_H

#include "data_token.h"

/* Column description */
typedef struct {
    char *name;
    DataType data_type;
} DataColumn;

/* Table scheme */
typedef struct {
    char *name;
    
    DataColumn **columns;
    size_t columns_count;
} DataSchema;

/* Data row */
typedef struct {
    size_t row_id;
    unsigned char flags;
    unsigned char *null_bm;
    DataToken **tokens;
    size_t count;
} DataRow;

typedef struct {
    DataSchema *scheme;
    DataRow **rows;
} DataTable;

#endif
