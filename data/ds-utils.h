#include "data-storage.h"

#ifndef DS_UTILS_H
#define DS_UTILS_H

char *dtok_bytes(DataToken *token);

char **dtokarr_bytes(DataTokenArray *array, toklen_t *out_count);
char *dtokarr_bytes_seq(DataTokenArray *array, toklen_t *out_size);

void freedtok(DataToken *token);
void freedtokarr(DataTokenArray *array);
void freeph(PageHeader *header);

#endif
