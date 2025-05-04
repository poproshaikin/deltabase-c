#include "../../data-storage.h"
#include <stdio.h>

#ifndef TOKEN_IO_H
#define TOKEN_IO_H

/* Writes a token from raw bytes */
int writedtok_v(const char *bytes, DataType type, FILE *dest);

int writedtok_d_v(const char *bytes, size_t size, DataType type, FILE *dest);

/* Writes a single DataToken to file */
int writedtok(DataToken *tkn, FILE *file);

/* Writes a token array from tokens */
int writedtokarr_v(DataToken **tokens, size_t count, FILE *file);

/* Writes a DataTokenArray to file */
int writedtokarr(DataTokenArray *arr, FILE *file);

size_t readlenprefix(FILE *file);

/* Reads a DataToken from file */
DataToken *readdtok(FILE *file);

char *readdtok_v(FILE *file, size_t *out_len);

/* Reads token array from file */
DataTokenArray *readdtokarr(FILE *file);

/* Inserts a DataToken into file at position */
int insdtok(DataToken *tkn, size_t pos, FILE *file);

/* Returns size of DataToken */
size_t dtoksize(const DataToken *tkn);

#endif
