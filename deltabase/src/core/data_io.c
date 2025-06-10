#include "include/data_token.h"
#include "include/utils.h"
#include "include/data_io.h"

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

size_t read_length_prefix(int fd) {
    size_t len;
    read(fd, &len, sizeof(size_t));
    return len;
}

static inline int write_dtype(DataType type, int fd) {
    return write(fd, &type, sizeof(DataType));
}

static inline int write_length_prefix(size_t size, int fd) {
    return write(fd, &size, sizeof(size_t));
}

static inline int write_dbuffer(const char *bytes, size_t size, int fd) {
    return write(fd, bytes, size);
}

int write_dt_v(const char *bytes, DataType type, int fd) {
    if (bytes == NULL || type == 0) {
        return -1;
    }

    size_t typeSize = dtype_size(type);

    write_dtype(type, fd);
    write_dbuffer(bytes, typeSize, fd);

    return 0;
}

int write_dt_dv(const char *bytes, size_t size, DataType type, int fd) {
    if (bytes == NULL || type == 0) {
        return -1;
    }

    write_dtype(type, fd);
    write_length_prefix(size, fd);
    write_dbuffer(bytes, size, fd);

    return 0;
}

int write_dt(const DataToken *token, int fd) {
    return dtype_size(token->type) == 0 ? 
        write_dt_dv(token->bytes, token->size, token->type, fd) :
        write_dt_v(token->bytes, token->type, fd);
}

static inline size_t read_dtype(int fd) {
    DataType type;
    int readd = read(fd, &type, sizeof(type));
    if (readd < 0) {
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
        case DT_NULL:
            return 0;
    }

    return -1;
}

int read_dt(DataToken *out, int fd) {
    DataType type = read_dtype(fd);
    printf("\n");
    if (type == 0) {
        printf("Failed to read token type\n");
        return 1;
    }
    printf("data type: %i\n", type);

    size_t finalSize = dtype_size(type);
    if (finalSize == 0) {
        printf("type has dynamic size\n");
        finalSize = read_length_prefix(fd);
    }
    printf("final size: %lu\n", finalSize);

    char *buffer = malloc(finalSize);
    if (!buffer) {
        printf("Failed to allocate memory\n");
        return 2;
    }

    size_t readd = read(fd, buffer, finalSize);
    if (readd < finalSize) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", finalSize, lseek(fd, 0, SEEK_CUR), fd);
        perror("Error");
        free(buffer);
        return 3;
    }
    printf("here\n");

    out->size = finalSize;
    out->bytes = buffer;
    out->type = type;

    printf("after\n");

    return 0;
}
//
// char *read_dt_v(int fd, size_t *out_len) {
//     DataType type = read_dtype(src);
//     if (type == 0) {
//         printf("Failed to read data type in readdtok_v\n");
//         return NULL;
//     }
//
//     size_t finalSize = dtype_size(type);
//     if (finalSize == 0) {
//         finalSize = read_length_prefix(src);
//     }
//
//     char *buffer = malloc(finalSize);
//     if (buffer == NULL) {
//         printf("Failed to allocate memory\n");
//         free(buffer);
//         return NULL;
//     }
//
//     size_t read = fread(buffer, sizeof(char), finalSize, src);
//     if (read < finalSize) {
//         printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", finalSize, ftell(src), fileno(src));
//         perror("Error");
//         free(buffer);
//         return NULL;
//     }
//
//     if (out_len != NULL) {
//         *out_len = read;
//     }
//
//     return buffer;
// }

int insert_dt(const DataToken *token, size_t pos, int fd) {
    lseek(fd, pos, SEEK_SET);   
    int writingPos = fmove(pos, token->size, fd);
    lseek(fd, writingPos, SEEK_SET);
    write_dt(token, fd);
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

int write_ph(const PageHeader *header, int fd) {
    write_dt_v((char*)&header->page_id, DT_LENGTH, fd);
    write_dt_v((char*)&header->rows_count, DT_LENGTH, fd);
    return 0;
}

int read_ph(PageHeader *out, int fd) {
    out->page_id = read_length_prefix(fd);
    out->rows_count = read_length_prefix(fd);
    return 0;
}

#define CHAR_BIT 8

/* Calculates null bitmap size in bytes */
static int nb_size(const DataSchema *schema) {
    return (schema->columns_count + CHAR_BIT - 1) / CHAR_BIT;
}

/* Calculates an amount of null tokens in a null bitmap */
static int nulls_count(unsigned char *nb, int len) {
    int count = 0;
    for (int i = 0; i < len; i++) {
        unsigned char byte = nb[i];
        for (int bit = 0; bit < 0; bit++) {
            if ((byte & (1 << bit)) == 1) {
                count++;
            }
        }
    }
    return count;
}

static unsigned char *build_nb(const DataSchema *schema, const DataToken **tokens) {
    unsigned char *bitmap = calloc(nb_size(schema), sizeof(char));

    for (int i = 0; i < schema->columns_count; i++) {
        if (tokens[i]->type == DT_NULL) {
            int byteIndex = i / CHAR_BIT;
            int bitIndex = i % CHAR_BIT;
            bitmap[byteIndex] |= (1 << bitIndex);
        }
    }

    return bitmap;
}

static inline int write_nb(const DataSchema *schema, const DataToken **tokens, int fd) {
    unsigned char *nb = build_nb(schema, tokens);
    int nbSize = nb_size(schema);
    return write(fd, nb, nbSize);
}

static inline bool is_null(const unsigned char *nb, size_t size, size_t bit) {
    size_t byte_index = bit / CHAR_BIT;
    size_t bit_index  = bit % size; 

    return (nb[byte_index] >> bit_index) & 1;
}
 
static inline size_t dr_size(const DataSchema *schema, const DataToken **tokens, int tokens_count) {
    int nbSize = nb_size(schema);
    size_t totalSize = nbSize;
    for (int i = 0; i < tokens_count; i++) {
        totalSize += dt_size(&*tokens[i]); 
    }
    return totalSize;
}

static inline unsigned char *read_nb(const DataSchema *schema, int fd) {
    int nb__size = nb_size(schema);
    unsigned char *bitmap = malloc(nb__size);
    read(fd, bitmap, nb__size);
    return bitmap;
}

static inline int write_dr_flags(unsigned char flags, int fd) {
    return write(fd, &flags, 1);
}

unsigned char read_dr_flags(int fd) {
    unsigned char flags[1];
    read(fd, flags, 1);
    return flags[0];
}

int write_dr_v(const DataSchema *schema, const DataToken **tokens, size_t count, unsigned char flags, int fd) {
    if (schema == NULL || tokens == NULL) {
        return -1;
    }

    size_t rowSize = dr_size(schema, tokens, count);

    write_length_prefix(rowSize, fd);
    write_dr_flags(flags, fd);
    write_nb(schema, tokens, fd);

    for (int i = 0; i < count; i++) {
        write_dt(tokens[i], fd);
    }

    return 0;
}

int write_dr(const DataSchema *schema, const DataRow *row, int fd) { 
    return write_dr_v(schema, (const DataToken **)row->tokens, row->count, row->flags, fd);
}

int read_dr(const DataSchema *schema, DataRow *out, int fd) {
    size_t row_size = read_length_prefix(fd);
    unsigned char flags = read_dr_flags(fd);
    unsigned char *nb = read_nb(schema, fd);
    int nb__size = nb_size(schema);
    int nulls = nulls_count(nb, nb__size);

    DataToken **tokens = malloc(schema->columns_count * sizeof(DataToken *));

    for (int i = 0; i < schema->columns_count; i++) {
        bool null = is_null(nb, nb__size, i);   
        if (null) {
            tokens[i] = make_token(DT_NULL, NULL, 0);
        }
        else {
            tokens[i] = malloc(sizeof(DataToken));
            read_dt(tokens[i], fd);    
        }   
    } 

    out->tokens = tokens;
    out->count = schema->columns_count;
    out->null_bm = nb;
    return 0;
}

