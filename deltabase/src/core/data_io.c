#include "include/data_table.h"
#include "include/data_token.h"
#include "include/utils.h"
#include "include/data_io.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int read_length_prefix(uint64_t *out, int fd) {
    if (read(fd, out, sizeof(uint64_t)) != sizeof(uint64_t)) {
        return 1;
    }
    return 0;
}

static inline int write_dtype(DataType type, int fd) {
    if (write(fd, &type, sizeof(DataType)) != sizeof(DataType)) {
        return 1;
    }
    return 0;
}

static inline int write_length_prefix(uint64_t size, int fd) {
    if (write(fd, &size, sizeof(uint64_t)) != sizeof(uint64_t)) {
        return 1;
    }
    return 0;
}

static inline int write_dbuffer(const char *bytes, uint64_t size, int fd) {
    if (write(fd, bytes, size) != size) {
        return 1;
    }
    return 0;
}

int read_uuid(char **out_buffer_ptr, int fd) {
    char *old_buffer = *out_buffer_ptr;

    *out_buffer_ptr = (char *)realloc(*out_buffer_ptr, 37);

    if (!*out_buffer_ptr) {
        fprintf(stderr, "Failed to reallocate memory while reading UUID\n");
        return 1;
    }

    ssize_t bytes_read = read(fd, *out_buffer_ptr, 36);
    if (bytes_read < 0) { 
        fprintf(stderr, "Failed to read data for UUID from file descriptor.\n");
        return 2;
    }

    if (bytes_read < 36) { 
        fprintf(stderr, "Warning: Read only %zd bytes for UUID, expected 36. File might be truncated or at EOF.\n", bytes_read);
        return 3; 
    }

    (*out_buffer_ptr)[36] = '\0'; 
    return 0;
}

int write_dt_v(const char *bytes, DataType type, int fd) {
    if (bytes == NULL || type == 0) {
        return 1;
    }

    uint64_t typeSize = dtype_size(type);

    if (write_dtype(type, fd) != 0) {
        return 2;
    }

    if (write_dbuffer(bytes, typeSize, fd) != 0) {
        return 3;
    }

    return 0;
}

int write_dt_dv(const char *bytes, uint64_t size, DataType type, int fd) {
    if (bytes == NULL || type == 0) {
        return 1;
    }

    if (write_dtype(type, fd) != 0) {
        return 2;
    }
    
    if (write_length_prefix(size, fd) != 0) {
        return 3;
    }
    
    if (write_dbuffer(bytes, size, fd) != 0) {
        return 4;
    }

    return 0;
}

int write_dt(const DataToken *token, int fd) {
    return dtype_size(token->type) == 0 ? 
        write_dt_dv(token->bytes, token->size, token->type, fd) :
        write_dt_v(token->bytes, token->type, fd);
}

static int read_dtype(DataType *out, int fd) {
    if (read(fd, out, sizeof(DataType)) != 0) {
        return 1;
    }
    return 0;
}

uint64_t dtype_size(DataType type) {
    switch (type) {
        case DT_INTEGER:
            return 4;
        case DT_REAL:
            return 8;
        case DT_BOOL:
            return 1;
        case DT_STRING:
            return 0;
        case DT_CHAR:
            return 4;
        case DT_NULL:
            return 0;
    }

    return -1;
}

int read_dt(DataToken *out, int fd) {
    DataType type;
    if (read_dtype(&type ,fd) != 0) {
        return 1;
    }
    printf("\n");
    if (type == 0) {
        printf("Failed to read token type\n");
        return 2;
    }
    uint64_t finalSize = dtype_size(type);
    if (finalSize == 0) {
        if (read_length_prefix(&finalSize, fd) != 0) {
            return 3;
        }
    }

    char *buffer = malloc(finalSize);
    if (!buffer) {
        printf("Failed to allocate memory\n");
        return 4;
    }

    uint64_t readd = read(fd, buffer, finalSize);
    if (readd < finalSize) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n", finalSize, lseek(fd, 0, SEEK_CUR), fd);
        perror("Error");
        free(buffer);
        return 5;
    }
    out->size = finalSize;
    out->bytes = buffer;
    out->type = type;

    return 0;
}

int insert_dt(const DataToken *token, uint64_t pos, int fd) {
    lseek(fd, pos, SEEK_SET);   
    int writingPos = fmove(pos, token->size, fd);
    lseek(fd, writingPos, SEEK_SET);
    write_dt(token, fd);
    return 0;
}

uint64_t dt_size(const DataToken *token) {
    uint64_t size = 0;
    size += sizeof(token->type); 
    uint64_t typeSize = dtype_size(token->type);

    if (typeSize == 0) {
        size += sizeof(uint64_t);
        size += token->size; 
    } else {
        size += typeSize;
    }

    return size;
}

static inline uint64_t ph_size(const PageHeader *header) {
    return sizeof(header->page_id) + sizeof(header->rows_count);
}

int write_ph(const PageHeader *header, int fd) {
    uint64_t size = ph_size(header); 
    size_t w = 0;

    w = write(fd, &size, sizeof(size));
    if (w != sizeof(size)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 1;
    }

    w = write(fd, &header->page_id, sizeof(header->page_id));
    if (w != sizeof(header->page_id)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 2;
    }

    w = write(fd, &header->rows_count, sizeof(header->rows_count));
    if (w != sizeof(header->rows_count)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 3;
    }

    w = write(fd, &header->min_rid, sizeof(header->min_rid));
    if (w != sizeof(header->min_rid)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 4;
    }

    w = write(fd, &header->max_rid, sizeof(header->max_rid));
    if (w != sizeof(header->max_rid)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 5;
    }

    return 0;
}

int read_ph(PageHeader *out, int fd) {
    // dont need info about size
    lseek(fd, sizeof(uint64_t), SEEK_CUR);

    if (read(fd, &out->page_id, sizeof(out->page_id)) != sizeof(out->page_id)) {
        fprintf(stderr,"Failed to read data in read_ph\n");
        return 1;
    }

    if (read(fd, &out->rows_count, sizeof(out->rows_count)) != sizeof(out->rows_count)) {
        fprintf(stderr,"Failed to read data in read_ph\n");
        return 2;
    }

    if (read(fd, &out->min_rid, sizeof(out->min_rid)) != sizeof(out->min_rid)) {
        fprintf(stderr,"Failed to read data in read_ph\n");
        return 3;
    }

    if (read(fd, &out->max_rid, sizeof(out->max_rid)) != sizeof(out->max_rid)) {
        fprintf(stderr,"Failed to read data in read_ph\n");
        return 4;
    }

    return 0;
}

int skip_ph(int fd) {
    uint64_t size;
    if (read(fd, &size, sizeof(size)) != sizeof(size)) {
        fprintf(stderr, "Failed to skip header\n");
        return 1;
    }

    lseek(fd, size, SEEK_CUR);
    return 0;
}

#define CHAR_BIT 8

/* Calculates null bitmap size in bytes */
static int nb_size(const MetaTable *schema) {
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

static unsigned char *build_nb(const MetaTable *schema, const DataToken **tokens) {
    unsigned char *bitmap = calloc(nb_size(schema), sizeof(char));
    if (!bitmap) {
        return NULL;
    }

    for (int i = 0; i < schema->columns_count; i++) {
        if (tokens[i]->type == DT_NULL) {
            int byteIndex = i / CHAR_BIT;
            int bitIndex = i % CHAR_BIT;
            bitmap[byteIndex] |= (1 << bitIndex);
        }
    }

    return bitmap;
}

static inline int write_nb(const MetaTable *schema, const DataToken **tokens, int fd) {
    unsigned char *nb = build_nb(schema, tokens);
    int nbSize = nb_size(schema);

    if (write(fd, nb, nbSize) != nbSize) {
        return 1;
    }

    return 0;
}

static inline bool is_null(const unsigned char *nb, uint64_t size, size_t bit) {
    uint64_t byte_index = bit / CHAR_BIT;
    uint64_t bit_index  = bit % size; 

    return (nb[byte_index] >> bit_index) & 1;
}
 
uint64_t dr_size_v(const MetaTable *schema, const DataToken **tokens, int tokens_count) {
    int nbSize = nb_size(schema);
    uint64_t totalSize = nbSize;
    for (int i = 0; i < tokens_count; i++) {
        totalSize += dt_size(&*tokens[i]); 
    }
    return totalSize;
}

uint64_t dr_size(const MetaTable *schema, const DataRow *row) {
    return dr_size_v(schema, (const DataToken **)row->tokens, row->count);
}

static inline unsigned char *read_nb(const MetaTable *schema, int fd) {
    int nb__size = nb_size(schema);
    unsigned char *bitmap = malloc(nb__size);

    if (!bitmap) {
        fprintf(stderr, "Failed to read null bitmap\n");
        return NULL;
    }

    if (read(fd, bitmap, nb__size) != nb__size) {
        fprintf(stderr, "Failed to read null bitmap\n");
        return NULL;
    }

    return bitmap;
}

static int write_dr_flags(DataRowFlags flags, int fd) {
    if (write(fd, &flags, sizeof(DataRowFlags)) != sizeof(DataRowFlags)) {
        return 1;
    }

    return 0;
}

int read_dr_flags(DataRowFlags *out, int fd) {
    if (read(fd, out, sizeof(DataRowFlags)) != sizeof(DataRowFlags)) {
        return 1;
    }
    return 0;
}

int write_dr_v(const MetaTable *schema, const DataToken **tokens, uint64_t count, unsigned char flags, int fd) {
    if (schema == NULL || tokens == NULL) {
        return -1;
    }

    uint64_t rowSize = dr_size_v(schema, tokens, count);

    if (write_length_prefix(rowSize, fd) != 0) {
        return 1;
    }
    if (write_dr_flags(flags, fd) != 0) {
        return 2;
    }
    if (write_nb(schema, tokens, fd) != 0) {
        return 3;
    }

    for (int i = 0; i < count; i++) {
        if (write_dt(tokens[i], fd) != 0) {
            return i + 3;
        }
    }

    return 0;
}

int write_dr(const MetaTable *schema, const DataRow *row, int fd) { 
    return write_dr_v(schema, (const DataToken **)row->tokens, row->count, row->flags, fd);
}

int read_dr(const MetaTable *schema, DataRow *out, int fd) {
    uint64_t row_size;
    if (read_length_prefix(&row_size, fd) != 0) {
        return 1;
    }

    DataRowFlags flags;
    if (read_dr_flags(&flags, fd) != 0) {
        return 2;
    }

    unsigned char *nb = read_nb(schema, fd);
    if (!nb) {
        return 3;
    }
    int nb__size = nb_size(schema);
    int nulls = nulls_count(nb, nb__size);

    DataToken **tokens = malloc(schema->columns_count * sizeof(DataToken *));
    if (!tokens) {
        return 4;
    }

    for (int i = 0; i < schema->columns_count; i++) {
        bool null = is_null(nb, nb__size, i);   
        if (null) {
            tokens[i] = make_token(DT_NULL, NULL, 0);
        }
        else {
            tokens[i] = malloc(sizeof(DataToken));
            if (read_dt(tokens[i], fd) != 0) {
                return 5;
            }
        }   
    } 

    out->tokens = tokens;
    out->count = schema->columns_count;
    return 0;
}


int write_mt(const MetaTable *schema, int fd) {
    ssize_t w;

    w = write(fd, schema->table_id, sizeof(schema->table_id));
    if (w != sizeof(schema->table_id)) return 1;

    uint64_t name_len = strlen(schema->name);
    w = write(fd, &name_len, sizeof(size_t));
    if (w != sizeof(size_t)) return 2;

    w = write(fd, &schema->has_pk, sizeof(schema->has_pk));
    if (w != sizeof(schema->has_pk)) return 3;

    w = write(fd, &schema->last_rid, sizeof(schema->last_rid));
    if (w != sizeof(schema->last_rid)) return 4;

    if (schema->has_pk) {
        w = write(fd, schema->pk, sizeof(schema->pk));
        if (w != sizeof(schema->pk)) return 5;
    }

    return 0;
}

int read_mt(MetaTable *out, int fd) {
    if (read(fd, out->table_id, sizeof(out->table_id)) != sizeof(out->table_id))
        return 1;

    uint64_t name_len;
    if (read(fd, &name_len, sizeof(size_t)) != sizeof(size_t))
        return 2;

    out->name = malloc(name_len + 1);
    if (!out->name)
        return 3;

    if (read(fd, out->name, name_len) != (ssize_t)name_len) {
        free(out->name);
        return 4;
    }

    out->name[name_len] = '\0';

    if (read(fd, &out->has_pk, sizeof(out->has_pk)) != sizeof(out->has_pk)) {
        free(out->name);
        return 5;
    }

    if (read(fd, &out->last_rid, sizeof(out->last_rid)) != sizeof(out->last_rid)) {
        free(out->name);
        return 6;
    }

    if (out->has_pk) {
        if (read(fd, out->pk, sizeof(out->pk)) != sizeof(out->pk)) {
            free(out->name);
            return 7;
        }
    }

    return 0;
}

static inline uint64_t mc_size(const MetaColumn *column) {
    return 
        sizeof(column->column_id) + 
        sizeof(uint64_t) + // length of a string
        strlen(column->name) +
        sizeof(column->data_type) +
        sizeof(column->flags);
}

int write_mc(const MetaColumn *column, int fd) {
    ssize_t w;

    uint64_t size = mc_size(column);
    w = write(fd, &size, sizeof(size));
    if (w != sizeof(size)) {
        return 1;
    }

    w = write(fd, column->column_id, sizeof(column->column_id));
    if (w != sizeof(column->column_id)) {
        return 2;
    }

    uint64_t name_len = strlen(column->name);
    w = write(fd, &name_len, sizeof(name_len));
    if (w != sizeof(name_len)) {
        return 3;   
    }

    w = write(fd, column->name, name_len);
    if (w != name_len) {
        return 4;
    }

    w = write(fd, &column->data_type, sizeof(column->data_type));
    if (w != sizeof(column->data_type)) {
        return 5;
    }

    w = write(fd, &column->flags, sizeof(column->flags));
    if (w != sizeof(column->flags)) {
        return 6;
    }

    return 0;
}

int read_mc(MetaColumn *column, int fd) {
    ssize_t r;
    uint64_t dcsize;

    r = read(fd, &dcsize, sizeof(dcsize));
    if (r != sizeof(dcsize)) {
        return 1;
    }

    r = read(fd, column->column_id, sizeof(column->column_id));
    if (r != sizeof(column->column_id)) {
        return 2;
    }

    uint64_t name_len;
    r = read(fd, &name_len, sizeof(name_len));
    if (r != sizeof(name_len)) {
        return 3;
    }

    column->name = malloc(name_len + 1);
    r = read(fd, column->name, name_len);
    if (r != name_len) {
        return 4;
    }
    column->name[name_len] = '\0';

    r = read(fd, &column->data_type, sizeof(column->data_type));
    if (r != sizeof(column->data_type)) {
        return 5;
    }

    r = read(fd, &column->flags, sizeof(column->flags));
    if (r != sizeof(column->flags)) {
        return 6;
    }

    return 0;
}
