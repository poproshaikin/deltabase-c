#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

/* Token length type */
typedef unsigned long int dulen_t;

/* Data types */
typedef enum {
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,
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
    dulen_t size; /* Used if the TYPE has dynamic size */
    char *bytes;
    DataType type;
} DataToken;

/* Array of data tokens */
typedef struct {
    DataToken **array;
    dulen_t count;
} DataTokenArray;

/* Column description */
typedef struct {
    char *name;
    DataType data_type;
    DataTypeSize size;
} Column;

/* Table scheme */
typedef struct {
    Column **columns;
    dulen_t columns_count;
} DataScheme;

/* Page header info */
typedef struct {
    dulen_t page_id;
    dulen_t rows_count;
    dulen_t data_start_offset;
} PageHeader;

/* Data row */
typedef struct {
    dulen_t row_id;
    unsigned char *null_bm;
    DataTokenArray *tokens;
} PageRow;

#endif
