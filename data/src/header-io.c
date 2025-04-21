#include "../data-storage.h"

#include <stdlib.h>

#include "../ds-utils.h"


int writeph(PageHeader *header, FILE *file) {
    writedtok_v((char*)&header->page_id, DTS_INTEGER, DT_INTEGER, file);
    writedtok_v((char*)&header->rows_count, DTS_INTEGER, DT_INTEGER, file);
    DataTokenArray *arr = newdta_from_values((void*)header->free_rows, DTS_INTEGER, header->rows_count, DTA_FIXED_SIZE, DT_INTEGER); 
    writedtokarr(arr, file);

    free(arr);

    return 0;
}

PageHeader *readph(FILE *file) {
    DataToken *pageId = readdtok(DTS_INTEGER, file);
    DataToken *rowsCount = readdtok(DTS_INTEGER, file);
    DataTokenArray *freeRows = readdtokarr_fs(DTS_INTEGER, file);

    PageHeader *header = malloc(sizeof(PageHeader));
    if (header == NULL) {
        printf("Failed to allocate memory at readph\n");
        freedtok(pageId);
        freedtok(rowsCount);
        freedtokarr(freeRows);
        return NULL;
    }

    header->page_id = *(int*)pageId->bytes;
    header->rows_count = *(int*)rowsCount->bytes;
    header->free_rows = (int*)dtokarr_bytes_seq(freeRows, NULL);
    header->free_rows_count = freeRows->count;

    return header;
}
