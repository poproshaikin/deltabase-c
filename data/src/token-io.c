#include "../data-storage.h"

#include <stdlib.h>
#include <sys/types.h>

#include "../../utils/stream-utils.h"
#include "internal.h"
#include "../data-storage-utils.h"

toklen_t readlenprefix(FILE *file) {
    toklen_t len;
    fread(&len, sizeof(toklen_t), 1, file);
    return len;
}

static int writedtype(DataType type, FILE *dest) {
    return fwrite(&type, sizeof(DataType), 1, dest);
}

static int writelenprefix(toklen_t size, FILE *dest) {
    return fwrite(&size, sizeof(toklen_t), 1, dest);
}

static int writedbuffer(const char *bytes, toklen_t size, FILE *dest) {
    return fwrite(bytes, 1, size, dest);
}

int writedtok_v(const char *bytes, toklen_t size, DataType type, FILE *dest) {
    if (bytes == NULL || type == 0) {
        return -1;
    }

    DataTypeSize typeSize = dtypesize(type);

    writedtype(type, dest);
    if (typeSize == DTS_DYNAMIC) 
        writelenprefix(size, dest);
    writedbuffer(bytes, size, dest);

    return 0;
}

int writedtok(DataToken *tkn, FILE *dest) {
    return writedtok_v(tkn->bytes, tkn->size, tkn->type, dest);
}

int writedtokarr_v(DataToken **tokens, toklen_t count, FILE *file) {
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
    return writedtokarr_v(tokens->array, tokens->count, file);
}

static DataType readdtype(FILE *src) {
    DataType type;
    int read = fread(&type, sizeof(type), 1, src);
    if (read < 0) {
        printf("Failed to read token type\n");
        return 0;
    }
    return type;
} 

DataToken *readdtok(FILE *src) {
    DataType type = readdtype(src);
    if (type == 0) {
        printf("Failed to read token type\n");
        return NULL;
    }

    toklen_t finalSize = dtypesize(type);
    if (finalSize == DTS_DYNAMIC) {
        finalSize = readlenprefix(src);
    }

    char *buffer = malloc(finalSize);
    if (buffer == NULL) {
        printf("Failed to allocate memory\n");
        free(buffer);
        return NULL;
    }

    int read = fread(buffer, sizeof(char), finalSize, src);
    if (read < sizeof(toklen_t)) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", finalSize, ftell(src), fileno(src));
        perror("Error");
        free(buffer);
        return NULL;
    }

    DataToken *token = malloc(sizeof(DataToken));
    token->size = finalSize;
    token->bytes = buffer;
    token->type = type;
    return token;
}

DataTokenArray *readdtokarr(FILE *file) {
    toklen_t elementCount = readlenprefix(file);
    DataToken **tokens = malloc(elementCount * sizeof(DataToken*));
    if (tokens == NULL) {
        printf("Failed to allocate memory at readdtokarr_fs\n");
        return NULL;
    }

    for (int i = 0; i < elementCount; i++) {
        tokens[i] = readdtok(file);
    }

    DataTokenArray *array = malloc(sizeof(DataTokenArray));
    if (array == NULL) {
        printf("Failed to allocate memory at readdtokarr_fs");
        free(tokens);
        return NULL;
    }

    array->array = tokens;
    array->count = elementCount;
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
