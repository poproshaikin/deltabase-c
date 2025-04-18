#include <stdio.h>
#include <sys/types.h>

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

typedef enum {
    TS_DYNAMIC = 0,
    TS_BYTE = 1,
    TS_FLOAT = 4,
    TS_INTEGER = 4,
} DataTokenSize;

typedef struct {
    DataTokenSize size;
    char *bytes;
} DataToken ;

u_long write_token(DataToken *tkn, FILE *dest);
DataToken *read_token(DataTokenSize size, FILE *src); 
int insert_token(DataToken *tkn, int pos, FILE *dest); 

#endif
