#include <stdlib.h>
#include "../token/token-io.h"
#include "../utils/utils.h"

int writeph(PageHeader *header, FILE *file) {
    writedtok_v((char*)&header->page_id, DT_LENGTH, file);
    writedtok_v((char*)&header->rows_count, DT_LENGTH, file);
    writedtok_v((char*)&header->data_start_offset, DT_LENGTH, file);
    return 0;
}

PageHeader *readph(FILE *file) {
    PageHeader *header = malloc(sizeof(PageHeader));
    if (header == NULL) {
        printf("Failed to allocate memory in readph\n");
        return NULL;
    }

    header->page_id = readlenprefix(file);
    header->rows_count = readlenprefix(file);
    header->data_start_offset = readlenprefix(file);

    return header;
}
