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
int writedtokarr_v(DataToken **tokens, toklen_t count, DataTokenArrayType type, FILE *file);

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
DataTokenArray *newdta_from_values(void *values, toklen_t el_size, toklen_t count, DataTokenArrayType arr_type, DataType el_type);

/* Creates a DataTokenArray from pointers */
DataTokenArray *newdta_from_ptrs(void **ptrs, toklen_t el_size, toklen_t count, DataTokenArrayType arr_type, DataType el_type);


/* Inserts a DataToken into file at position */
int insdtok(DataToken *tkn, toklen_t pos, FILE *file);


/* Returns size of DataToken */
toklen_t dtoksize(const DataToken *tkn);

#endif
