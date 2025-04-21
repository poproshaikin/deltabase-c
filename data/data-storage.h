#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

/* Token length type */
typedef unsigned long int toklen_t;

/* Data types */
typedef enum {
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,
    DT_STRING = 100,
} DataType;

/* Data type sizes */
typedef enum {
    DTS_CHAR = 1,
    DTS_INTEGER = 4,
    DTS_REAL = 8,
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

/* Number of tokens in page header */
#define HEADER_TOKENS_COUNT 3

/* Page header info */
typedef struct {
    int page_id;
    int rows_count;
    int *free_rows;
    int free_rows_count;
} PageHeader;

/* Data row */
typedef struct {
    int row_id;
    DataTokenArray *tokens;
} PageRow;

/* Table scheme */
typedef struct {
    Column **columns;
    int columns_count;
} DataScheme;

/* Reads a length prefix from file */
toklen_t readlenprefix(FILE *file);


/* Writes a token from raw bytes */
int writedtok_v(const char *bytes, toklen_t size, DataType type, FILE *dest);

/* Writes a single DataToken to file */
int writedtok(DataToken *tkn, FILE *file);

/* Writes a token array from tokens */
int writedtokarr_v(DataToken **tokens, int count, DataTokenArrayType type, FILE *file);

/* Writes a DataTokenArray to file */
int writedtokarr(DataTokenArray *arr, FILE *file);


/* Reads a DataToken from file */
DataToken *readdtok(toklen_t size, FILE *file);

/* Reads fixed-size token array from file */
DataTokenArray *readdtokarr_fs(toklen_t el_size, FILE *file);

/* Reads dynamic-size token array from file */
DataTokenArray *readdtokarr_ds(FILE *file);


/* Creates a new DataToken from bytes */
DataToken *newdtok(char *bytes, toklen_t size, DataType data_type);

/* Creates a DataTokenArray from value block */
DataTokenArray *newdta_from_values(void *values, toklen_t el_size, int count, DataTokenArrayType arr_type, DataType el_type);

/* Creates a DataTokenArray from pointers */
DataTokenArray *newdta_from_ptrs(void **ptrs, toklen_t el_size, int count, DataTokenArrayType arr_type, DataType el_type);


/* Inserts a DataToken into file at position */
int insdtok(DataToken *tkn, int pos, FILE *file);


/* Returns size of DataToken */
toklen_t dtoksize(const DataToken *tkn);


/* Writes a row of tokens to file */
int writedrow(DataScheme *scheme, DataToken *tkns, FILE *file);

/* Reads a row of tokens from file */
DataToken **readdrow(DataScheme *scheme, FILE *file);


/* Writes a page header to file */
int writeph(PageHeader *header, FILE *file);

/* Reads a page header from file */
PageHeader *readph(FILE *file);

#endif
