
#include "../data-storage-utils.h"
#include <stdlib.h>
#include <string.h>

char *dtok_bytes(DataToken *token) {
    char *buffer = token->bytes;
    buffer = realloc(buffer, (token->size + 1) * sizeof(char));
    buffer[token->size + 1] = '\0';
    return buffer;
}

char **dtokarr_bytes(DataTokenArray *array, toklen_t *out_count) { 
    char **values = malloc(array->count * sizeof(char *));
    if (values == NULL) {
        printf("Failed to allocate memory in dtokarr_bytes\n");
        return NULL;
    }
    for (int i = 0; i < array->count; i++) {
        values[i] = array->array[i]->bytes;
    }

    if (out_count != NULL) *out_count = array->count;
    return values;
}

char *dtokarr_bytes_seq(DataTokenArray *array, toklen_t *out_size) {
    toklen_t totalBytes = 0;
    for (int i = 0; i < array->count; i++) {
        totalBytes += array->array[i]->size;
    }
    
    char *buffer = malloc(totalBytes * sizeof(char));
    if (buffer == NULL) {
        printf("Failed to allocate memory in dtokarr_bytes_seq\n");
        return NULL;
    }
    
    char *ptr = buffer;
    for (toklen_t i = 0; i < array->count; i++) {
        DataToken *token = array->array[i];
        memcpy(ptr, token->bytes, token->size);
        ptr += token->size;
    }

    if (out_size != NULL) *out_size = totalBytes;
    return buffer;
}

void freedtok(DataToken *token) {
    free(token->bytes);
    free(token);
}

void freedtokarr(DataTokenArray *array) {
    for (int i = 0; i < array->count; i++) {
        free(array->array[i]);
    }
    free(array);
}

void freeph(PageHeader *header) {
    free(header->free_rows);
    free(header);
}

DataTypeSize dtypesize(DataType type) {
    DataTypeSize size;

    switch (type) {
        case DT_INTEGER:
            size = DTS_INTEGER;
            break;
        case DT_REAL:
            size = DTS_REAL;
            break;
        case DT_CHAR:
            size = DTS_CHAR;
            break;
        case DT_LENGTH:
            size = DTS_LENGTH;
            break;
        case DT_STRING:
            size = DTS_DYNAMIC;
            break;
    }

    return size;
}
