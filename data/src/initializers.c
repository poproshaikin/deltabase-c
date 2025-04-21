#include "../data-storage.h"

#include <stdlib.h>
#include <string.h>

DataToken *newdtok(char *bytes, toklen_t size, DataType data_type) {
    DataToken *token = malloc(sizeof(DataToken));
    if (token == NULL) {
        printf("Failed to allocate memory in newdtok\n");
        return NULL;
    }
    token->bytes = malloc(size * sizeof(char));
    if (token->bytes == NULL) {
        printf("Failed to allocate memory in newdtok\n");
        return NULL;
    }
    memcpy(token->bytes, bytes, size);
    token->size = size;
    token->type = data_type;
    return token;
}

DataTokenArray *newdta_from_values(void *values, toklen_t el_size, int count, DataTokenArrayType arr_type, DataType el_type) {
    DataToken **tokens = malloc(count * sizeof(DataToken*));
    if (tokens == NULL) {
        printf("Failed to allocate memory in newdta_from_values\n");
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        tokens[i] = newdtok(((char*)values) + i * el_size, el_size, el_type);
    }

    DataTokenArray *array = malloc(sizeof(DataTokenArray));
    if (array == NULL) {
        printf("Failed to allocate memory in newdta_from_values\n");
        for (int i = 0; i < count; i++) {
            free(tokens[i]);
        }
        free(tokens);
        return NULL;
    }

    array->array = tokens;
    array->count = count;
    array->type = arr_type;
    return array;
}

DataTokenArray *newdta_from_ptrs(void **ptrs, toklen_t el_size, int count, DataTokenArrayType arr_type, DataType el_type) {
    DataToken **tokens = malloc(count * sizeof(DataToken*));
    if (tokens == NULL) {
        printf("Failed to allocate memory in newdta");
        return NULL;
    }

    for (int i = 0; i < count; i++) {
        tokens[i] = newdtok(((char**)ptrs)[i], el_size, el_type);
    }

    DataTokenArray *array = malloc(sizeof(DataTokenArray));
    if (array == NULL) {
        printf("Failed to allocate memory in newdta_from_values\n");
        for (int i = 0; i < count; i++) {
            free(tokens[i]);
        }
        free(tokens);
        return NULL;
    }

    array->array = tokens;
    array->count = count;
    array->type = arr_type;
    return array;
}
