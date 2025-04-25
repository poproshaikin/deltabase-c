#include <stdlib.h>
#include "../token/token-io.h"
#include "../utils/utils.h"

int writeph(PageHeader *header, FILE *file) {
    writedtok_v((char*)&header->page_id, DTS_LENGTH, DT_LENGTH, file);
    writedtok_v((char*)&header->rows_count, DTS_LENGTH, DT_LENGTH, file);

    DataTokenArray *arr = newdtokarr_from_values((void*)header->free_rows, 
            DTS_LENGTH, 
            header->free_rows_count, 
            DT_LENGTH); 

    writedtokarr(arr, file);

    free(arr);

    return 0;
}

PageHeader *readph(FILE *file) {
    DataToken *pageId = readdtok(file);
    DataToken *rowsCount = readdtok(file);
    DataTokenArray *freeRows = readdtokarr(file);
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
