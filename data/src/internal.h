#include "../data-storage.h"

#ifndef DS_INTERNAL_H
#define DS_INTERNAL_H

/* Reads a length prefix from file */
toklen_t readlenprefix(FILE *file);


/* Writes a token from raw bytes */
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


/* Writes a row of tokens to file */
int writedrow(DataScheme *scheme, DataToken *tkns, int tokens_count, FILE *file);

/* Reads a row of tokens from file */
DataToken **readdrow(DataScheme *scheme, FILE *file);


/* Inserts a DataToken into file at position */
int insdtok(DataToken *tkn, toklen_t pos, FILE *file);

/* Returns size of DataToken */
toklen_t dtoksize(const DataToken *tkn);

#endif
