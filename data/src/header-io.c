#include "internal.h"

#include <stdlib.h>

#include "../data-storage-utils.h"

int writeph(PageHeader *header, FILE *file) {
    writedtok_v((char*)&header->page_id, DTS_LENGTH, DT_LENGTH, file);
    writedtok_v((char*)&header->rows_count, DTS_LENGTH, DT_LENGTH, file);

    DataTokenArray *arr = newdta_from_values((void*)header->free_rows, 
            DTS_LENGTH, 
            header->free_rows_count, 
            DTA_FIXED_SIZE, 
            DT_LENGTH); 

    writedtokarr(arr, file);

    free(arr);

    return 0;
}

PageHeader *readph(FILE *file) {
    DataToken *pageId = readdtok(DTS_LENGTH, file);
    DataToken *rowsCount = readdtok(DTS_LENGTH, file);
    DataTokenArray *freeRows = readdtokarr_fs(DTS_LENGTH, file);
    if (freeRows == NULL) {
        printf("Failed to read tokens array\n");
        freedtok(pageId);
        freedtok(rowsCount);
        return NULL;
    }

    PageHeader *header = malloc(sizeof(PageHeader));
    if (header == NULL) {
        printf("Failed to allocate memory at readph\n");
        freedtok(pageId);
        freedtok(rowsCount);
        freedtokarr(freeRows);
        return NULL;
    }

    header->page_id = *(toklen_t*)pageId->bytes;
    header->rows_count = *(toklen_t*)rowsCount->bytes;
    header->free_rows = (toklen_t*)dtokarr_bytes_seq(freeRows, NULL);
    header->free_rows_count = freeRows->count;

    return header;
}
