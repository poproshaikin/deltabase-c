#include "../data-storage.h"

#include <stdlib.h>
#include <sys/types.h>

#include "../../utils/stream-utils.h"
#include "internal.h"

toklen_t readlenprefix(FILE *file) {
    toklen_t len;
    fread(&len, sizeof(toklen_t), 1, file);
    return len;
}

int writedtok_v(const char *bytes, toklen_t size, DataType type, FILE *dest) {
    if (type == 0) {
        return -1;
    }
    if (type > 0 && type < 100) {
        fwrite(bytes, sizeof(char), size, dest);
        return 0;
    } 
    else if (type >= 100 && type < 1000) {
        fwrite(&size, sizeof(toklen_t), 1, dest);    
        fwrite(bytes, sizeof(char), size, dest);
        return 0;
    }
    printf("Cannot write token: unknown type\n");
    return -2;
}

int writedtok(DataToken *tkn, FILE *dest) {
    return writedtok_v(tkn->bytes, tkn->size, tkn->type, dest);
}

int writedtokarr_v(DataToken **tokens, toklen_t count, DataTokenArrayType type, FILE *file) {
    if (tokens == NULL || count == 0) {
        return 0;
    }

    writedtok_v((char*)&count, DTS_LENGTH, DT_LENGTH, file);
    for (int i = 0; i < count; i++) {
        writedtok(tokens[i], file);
    }

    return 0;
}

int writedtokarr(DataTokenArray *tokens, FILE *file) {
    return writedtokarr_v(tokens->array, tokens->count, tokens->type, file);
}

DataToken *readdtok(toklen_t size, FILE *src) {
    char *buffer = malloc(sizeof(char) * size);
    if (buffer == NULL) {
        printf("Failed to allocate memory");
        free(buffer);
        return NULL;
    }

    int read = fread(buffer, sizeof(char), size, src);
    if (read < 0) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", size, ftell(src), fileno(src));
        perror("Error");
        free(buffer);
        return NULL;
    }

    DataToken *token = malloc(sizeof(DataToken));
    if (token == NULL) {
        printf("Failed to allocate memory");
        free(buffer);
        return NULL;
    }

    token->size = size;
    token->bytes = buffer;
    return token;
}

DataTokenArray *readdtokarr_fs(toklen_t el_size, FILE *file) {
    toklen_t elementCount = readlenprefix(file);
    DataToken **tokens = malloc(elementCount * sizeof(DataToken*));
    if (tokens == NULL) {
        printf("Failed to allocate memory at readdtokarr_fs\n");
        return NULL;
    }

    for (int i = 0; i < elementCount; i++) {
        tokens[i] = readdtok(el_size, file);
    }

    DataTokenArray *array = malloc(sizeof(DataTokenArray));
    if (array == NULL) {
        printf("Failed to allocate memory at readdtokarr_fs");
        free(tokens);
        return NULL;
    }

    array->array = tokens;
    array->count = elementCount;
    array->type = DTA_FIXED_SIZE;
    return array;
}

DataTokenArray *readdtokarr_ds(FILE *file) {
    toklen_t elementCount = readlenprefix(file);
    DataToken **tokens = malloc(elementCount * sizeof(DataToken*));
    if (tokens == NULL) {
        printf("Failed to allocate memory at readdtokarr_ds");
        free(tokens);
        return NULL;
    }

    for (int i = 0; i < elementCount; i++) {
        toklen_t elSize = readlenprefix(file);
        tokens[i] = readdtok(elSize, file);
    }

    DataTokenArray *array = malloc(sizeof(DataTokenArray));
    array->array = tokens;
    array->count = elementCount;
    array->type = DTA_DYNAMIC_SIZE;
    return array;
}

int insdtok(DataToken *tkn, toklen_t pos, FILE *file) {
    fseek(file, pos, SEEK_SET);   
    int writingPos = fmove(pos, tkn->size, file);
    fseek(file, writingPos, SEEK_SET);
    writedtok(tkn, file);
    return 0;
}

toklen_t dtoksize(const DataToken *tkn) {
    return tkn->size;
}
