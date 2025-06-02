#include "include/core.h"
#include "include/data_token.h"
#include "include/utils.h"

#include <stdlib.h>

size_t read_length_prefix(FILE *file) {
    size_t len;
    fread(&len, sizeof(size_t), 1, file);
    return len;
}

static inline int write_dtype(DataType type, FILE *dest) {
    return fwrite(&type, sizeof(DataType), 1, dest);
}

static inline int write_length_prefix(size_t size, FILE *dest) {
    return fwrite(&size, sizeof(size_t), 1, dest);
}

static inline int write_dbuffer(const char *bytes, size_t size, FILE *dest) {
    return fwrite(bytes, 1, size, dest);
}

int write_dt_v(const char *bytes, DataType type, FILE *dest) {
    if (bytes == NULL || type == 0) {
        return -1;
    }

    size_t typeSize = dtype_size(type);

    write_dtype(type, dest);
    write_dbuffer(bytes, typeSize, dest);

    return 0;
}

int write_dt_dv(const char *bytes, size_t size, DataType type, FILE *dest) {
    if (bytes == NULL || type == 0) {
        return -1;
    }

    write_dtype(type, dest);
    write_length_prefix(size, dest);
    write_dbuffer(bytes, size, dest);

    return 0;
}

int write_dt(DataToken *token, FILE *dest) {
    return dtype_size(token->type) == 0 ? 
        write_dt_dv(token->bytes, token->size, token->type, dest) :
        write_dt_v(token->bytes, token->type, dest);
}

static inline size_t read_dtype(FILE *src) {
    DataType type;
    int read = fread(&type, sizeof(type), 1, src);
    if (read < 0) {
        printf("Failed to read token type\n");
        return 0;
    }
    return type;
}

size_t dtype_size(DataType type) {
    switch (type) {
        case DT_INTEGER:
            return 4;
        case DT_REAL:
            return 8;
        case DT_BOOL:
            return 1;
        case DT_STRING:
            return 0;
        case DT_LENGTH:
            return 8;
        case DT_CHAR:
            return 4;
    }

    return -1;
}

int read_dt(DataToken *out, FILE *src) {
    DataType type = read_dtype(src);
    if (type == 0) {
        printf("Failed to read token type\n");
        return 1;
    }

    size_t finalSize = dtype_size(type);
    if (finalSize == 0) {
        finalSize = read_length_prefix(src);
    }

    char *buffer = malloc(finalSize);
    if (buffer == NULL) {
        printf("Failed to allocate memory\n");
        free(buffer);
        return 2;
    }

    size_t read = fread(buffer, sizeof(char), finalSize, src);
    if (read < finalSize) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", finalSize, ftell(src), fileno(src));
        perror("Error");
        free(buffer);
        return 3;
    }

    out->size = finalSize;
    out->bytes = buffer;
    out->type = type;

    return 0;
}

char *read_dt_v(FILE *src, size_t *out_len) {
    DataType type = read_dtype(src);
    if (type == 0) {
        printf("Failed to read data type in readdtok_v\n");
        return NULL;
    }

    size_t finalSize = dtype_size(type);
    if (finalSize == 0) {
        finalSize = read_length_prefix(src);
    }

    char *buffer = malloc(finalSize);
    if (buffer == NULL) {
        printf("Failed to allocate memory\n");
        free(buffer);
        return NULL;
    }

    size_t read = fread(buffer, sizeof(char), finalSize, src);
    if (read < finalSize) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", finalSize, ftell(src), fileno(src));
        perror("Error");
        free(buffer);
        return NULL;
    }

    if (out_len != NULL) {
        *out_len = read;
    }

    return buffer;
}

int insert_dt(DataToken *token, size_t pos, FILE *file) {
    fseek(file, pos, SEEK_SET);   
    int writingPos = fmove(pos, token->size, file);
    fseek(file, writingPos, SEEK_SET);
    write_dt(token, file);
    return 0;
}

size_t dt_size(const DataToken *token) {
    size_t size = 0;
    size += sizeof(token->type); 
    size_t typeSize = dtype_size(token->type);

    if (typeSize == 0) {
        size += dtype_size(DT_LENGTH);
        size += token->size; 
    } else {
        size += typeSize;
    }

    return size;
}

int write_ph(PageHeader *header, FILE *src) {
    write_dt_v((char*)&header->page_id, DT_LENGTH, src);
    write_dt_v((char*)&header->rows_count, DT_LENGTH, src);
    return 0;
}

int read_ph(PageHeader *out, FILE *src) {
    out->page_id = read_length_prefix(src);
    out->rows_count = read_length_prefix(src);
    return 0;
}

