#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

typedef unsigned long int toklen_t;

typedef enum {
    DT_INTEGER = 1,
    DT_REAL,
    DT_CHAR,

    DT_STRING = 100,
} DataType;

typedef enum {
    DTS_CHAR = 1,
    DTS_INTEGER = 4,
    DTS_REAL = 8,
} DataTypeSize;

typedef struct {
    toklen_t size;
    char *bytes;
    DataType type;
} DataToken;

toklen_t dtoksize(const DataToken *tkn);

typedef struct {
    char *name;
    DataType data_type;
    DataTypeSize size;
} Column;

typedef struct {
    Column *columns;
    int columns_count;
} DataScheme;

u_long writedtok(DataToken *tkn, FILE *dest);
DataToken *readdtok(toklen_t size, FILE *src); 
int insdtok(DataToken *tkn, int pos, FILE *dest); 


int writedrow(DataScheme *scheme, DataToken *tkns, FILE *file);
DataToken *readdrow(DataScheme *scheme, FILE *file);

#endif
