#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>

/* Data types */
typedef enum {
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,
    DT_BOOL,
    DT_LENGTH,
    DT_STRING,
} DataType;

/* Data type sizes */
typedef enum {
    DTS_UNDEFINED = 0,

    DTS_CHAR = 1,
    DTS_INTEGER = 4,
    DTS_REAL = 8,
    DTS_LENGTH = 8,

    DTS_DYNAMIC = -1
} DataTypeSize;

/* Single data token */
typedef struct {
    size_t size; /* Used if the TYPE has dynamic size */
    char *bytes;
    DataType type;
} DataToken;

/* Array of data tokens */
typedef struct {
    DataToken **array;
    size_t count;
} DataTokenArray;

/* Column description */
typedef struct {
    char *name;
    DataType data_type;
} DataColumn;

/* Table scheme */
typedef struct {
    DataColumn **columns;
    size_t columns_count;
} DataScheme;

/* Page header info */
typedef struct {
    size_t page_id;
    size_t rows_count;
    size_t data_start_offset;
} PageHeader;

/* Data row */
typedef struct {
    size_t row_id;
    unsigned char *null_bm;
    DataTokenArray *tokens;
} PageRow;

#endif
