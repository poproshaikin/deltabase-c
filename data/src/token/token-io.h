#include "../../data-storage.h"

#ifndef TOKEN_IO_H
#define TOKEN_IO_H

/* Writes a token from raw bytes */
#include <stdio.h>
int writedtok_v(const char *bytes, toklen_t size, DataType type, FILE *dest);

/* Writes a single DataToken to file */
int writedtok(DataToken *tkn, FILE *file);

/* Writes a token array from tokens */
int writedtokarr_v(DataToken **tokens, toklen_t count, FILE *file);

/* Writes a DataTokenArray to file */
int writedtokarr(DataTokenArray *arr, FILE *file);


/* Reads a DataToken from file */
DataToken *readdtok(FILE *file);

/* Reads token array from file */
DataTokenArray *readdtokarr(FILE *file);


/* Inserts a DataToken into file at position */
int insdtok(DataToken *tkn, toklen_t pos, FILE *file);

/* Returns size of DataToken */
toklen_t dtoksize(const DataToken *tkn);

#endif
