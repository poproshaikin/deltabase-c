#include "../data-storage.h"

#include <limits.h>
#include <stdlib.h>

#include "internal.h"

static int nullbitmapsize(DataScheme *scheme) {
    return (scheme->columns_count + CHAR_BIT - 1) / CHAR_BIT;
}

static char *buildnullbitmap(DataScheme *scheme, DataToken *tkns) {
    char *bitmap = calloc(nullbitmapsize(scheme), sizeof(char));

    for (int i = 0; i < scheme->columns_count; i++) {
        if (tkns[i].bytes == NULL) {
            int byteIndex = i / CHAR_BIT;
            int bitIndex = i % CHAR_BIT;
            bitmap[byteIndex] |= (1 << bitIndex);
        }
    }

    return bitmap;
}

static toklen_t drowsize(DataScheme *scheme, DataToken *tokens) {
    toklen_t totalSize = 0;
    totalSize += nullbitmapsize(scheme);
    for (int i = 0; i < scheme->columns_count; i++) {
        toklen_t size = dtoksize(&tokens[i]); 
        printf("token size: %lu\n", size);
        totalSize += size;
    }
    return totalSize;
}

static char *readnullbitmap(DataScheme *scheme, FILE *file) {
    int nbSize = nullbitmapsize(scheme);
    char *bitmap = malloc(nbSize * sizeof(char));
    fread(bitmap, sizeof(char), nbSize, file);
    return bitmap;
}

int writedrow(DataScheme *scheme, DataToken *tokens, FILE *file) {
    if (scheme == NULL || tokens == NULL || file == NULL) {
        return -1;
    }
    toklen_t rowSize = drowsize(scheme, tokens);
    fwrite(&rowSize, sizeof(toklen_t), 1, file);

    char *nb = buildnullbitmap(scheme, tokens);
    fwrite(nb, sizeof(char), nullbitmapsize(scheme), file);

    for (int i = 0; i < scheme->columns_count; i++) {
        writedtok(&tokens[i], file);
    }

    return 0;
}

DataToken **readdrow(DataScheme *scheme, FILE *file) {
    if (scheme == NULL || file == NULL) {
        return NULL;
    }

    toklen_t rowSize = readlenprefix(file);
    char *nb = readnullbitmap(scheme, file);
    DataToken **tokens = malloc(scheme->columns_count * sizeof(DataToken*));

    for (int i = 0; i < scheme->columns_count; i++) {
        toklen_t len;
        if (scheme->columns[i]->data_type >= DT_STRING) {
            len = readlenprefix(file);
        }
        else {
            len = scheme->columns[i]->size;
        }

        DataToken *token = readdtok(len, file);
        if (token == NULL) {
            printf("Failed to read %i token", i);
            free(tokens);
            free(nb);
            return NULL;
        }

        token->type = scheme->columns[i]->data_type;
        tokens[i] = token;
    }

    return tokens;
}
