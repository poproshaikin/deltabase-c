#include "data-storage.h"

#ifndef DS_UTILS_H
#define DS_UTILS_H

char *dtok_bytes(DataToken *token);

char **dtokarr_bytes(DataTokenArray *array, toklen_t *out_count);
char *dtokarr_bytes_seq(DataTokenArray *array, toklen_t *out_size);

void freedtok(DataToken *token);
void freedtokarr(DataTokenArray *array);
void freeph(PageHeader *header);

/* Creates a new DataToken from bytes */
DataToken *newdtok(char *bytes, toklen_t size, DataType data_type);

/* Creates a DataTokenArray from value block */
DataTokenArray *newdtokarr_from_values(void *values, toklen_t el_size, toklen_t count, DataType el_type);

/* Creates a DataTokenArray from pointers */
DataTokenArray *newdtokarr_from_ptrs(void **ptrs, toklen_t el_size, toklen_t count, DataType el_type);

DataTypeSize dtypesize(DataType type);

#endif
