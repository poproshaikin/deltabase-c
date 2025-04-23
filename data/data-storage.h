#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

/* Token length type */
typedef unsigned long int toklen_t;

/* Data types */
typedef enum {
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,
    DT_LENGTH,
    DT_STRING = 100,
} DataType;

/* Data type sizes */
typedef enum {
    DTS_CHAR = 1,
    DTS_INTEGER = 4,
    DTS_REAL = 8,
    DTS_LENGTH = 8
} DataTypeSize;

/* Token array type */
typedef enum {
    DTA_FIXED_SIZE = 1,
    DTA_DYNAMIC_SIZE,
} DataTokenArrayType;

/* Single data token */
typedef struct {
    toklen_t size;
    char *bytes;
    DataType type;
} DataToken;

/* Array of data tokens */
typedef struct {
    DataToken **array;
    toklen_t count;
    DataTokenArrayType type;
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
    toklen_t columns_count;
} DataScheme;

/* Page header info */
typedef struct {
    toklen_t page_id;
    toklen_t rows_count;
    toklen_t *free_rows;
    toklen_t free_rows_count;
} PageHeader;

/* Data row */
typedef struct {
    toklen_t row_id;
    DataTokenArray *tokens;
} PageRow;

/* Writes a page header to file */
int writeph(PageHeader *header, FILE *file);

/* Reads a page header from file */
PageHeader *readph(FILE *file);

/* Writes a row of tokens to file */
int writedrow(DataScheme *scheme, DataToken *tkns, FILE *file);

/* Reads a row of tokens from file */
DataToken **readdrow(DataScheme *scheme, FILE *file);

#endif
