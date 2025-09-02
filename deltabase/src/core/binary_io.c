#include "include/binary_io.h"
#include <fcntl.h>
#include <stdio.h>
#include "include/utils.h"
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

int
read_length_prefix(uint64_t* out, int fd) {
    if (read(fd, out, sizeof(uint64_t)) != sizeof(uint64_t)) {
        return 1;
    }
    return 0;
}

static inline int
write_dtype(DataType type, int fd) {
    if (write(fd, &type, sizeof(DataType)) != sizeof(DataType)) {
        return 1;
    }
    return 0;
}

static inline int
write_length_prefix(uint64_t size, int fd) {
    if (write(fd, &size, sizeof(uint64_t)) != sizeof(uint64_t)) {
        return 1;
    }
    return 0;
}

static inline int
write_dbuffer(const char* bytes, uint64_t size, int fd) {
    if (write(fd, bytes, size) != size) {
        return 1;
    }
    return 0;
}

int
read_uuid(char** out_buffer_ptr, int fd) {
    char* old_buffer = *out_buffer_ptr;

    *out_buffer_ptr = (char*)realloc(*out_buffer_ptr, 37);

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
        fprintf(stderr,
                "Warning: Read only %zd bytes for UUID, expected 36. File might be truncated or at "
                "EOF.\n",
                bytes_read);
        return 3;
    }

    (*out_buffer_ptr)[36] = '\0';
    return 0;
}

int
write_dt_v(const char* bytes, DataType type, int fd) {
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

int
write_dt_dv(const char* bytes, uint64_t size, DataType type, int fd) {
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

int
write_dt(const DataToken* token, int fd) {
    return dtype_size(token->type) == 0 ? write_dt_dv(token->bytes, token->size, token->type, fd)
                                        : write_dt_v(token->bytes, token->type, fd);
}

static int
read_dtype(DataType* out, int fd) {
    if (read(fd, out, sizeof(DataType)) != sizeof(DataType)) {
        return 1;
    }
    return 0;
}

uint64_t
dtype_size(DataType type) {
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
    default:
        return -1;
    }
}

int
read_dt(DataToken* out, int fd) {
    DataType type;
    if (read_dtype(&type, fd) != 0) {
        return 1;
    }
    if (type == 0) {
        fprintf(stderr, "Failed to read token type\n");
        return 2;
    }
    uint64_t finalSize = dtype_size(type);
    if (finalSize == 0) {
        if (read_length_prefix(&finalSize, fd) != 0) {
            return 3;
        }
    }

    char* buffer = malloc(finalSize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 4;
    }

    uint64_t readd = read(fd, buffer, finalSize);
    if (readd < finalSize) {
        printf("Failed to read token. Size: %lu. Pos: %li. Fd: %i\n",
               finalSize,
               lseek(fd, 0, SEEK_CUR),
               fd);
        perror("Error");
        free(buffer);
        return 5;
    }

    out->size = finalSize;
    out->bytes = buffer;
    out->type = type;

    return 0;
}

int
insert_dt(const DataToken* token, uint64_t pos, int fd) {
    lseek(fd, pos, SEEK_SET);
    int writingPos = fmove(pos, token->size, fd);
    lseek(fd, writingPos, SEEK_SET);
    write_dt(token, fd);
    return 0;
}

uint64_t
dt_size(const DataToken* token) {
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

static inline uint64_t
ph_size(const PageHeader* header) {
    return sizeof(header->page_id) + sizeof(header->rows_count) + sizeof(header->min_rid) +
           sizeof(header->max_rid);
}

int
write_ph(const PageHeader* header, int fd) {
    if (!header) {
        return 1;
    }

    uint64_t size = ph_size(header);
    size_t w = 0;

    w = write(fd, &size, sizeof(size));
    if (w != sizeof(size)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 2;
    }

    w = write(fd, &header->page_id, sizeof(header->page_id));
    if (w != sizeof(header->page_id)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 3;
    }

    w = write(fd, &header->rows_count, sizeof(header->rows_count));
    if (w != sizeof(header->rows_count)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 4;
    }

    w = write(fd, &header->min_rid, sizeof(header->min_rid));
    if (w != sizeof(header->min_rid)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 5;
    }

    w = write(fd, &header->max_rid, sizeof(header->max_rid));
    if (w != sizeof(header->max_rid)) {
        fprintf(stderr, "Failed to write data in write_ph\n");
        return 6;
    }

    return 0;
}

int
read_ph(PageHeader* out, int fd) {
    // dont need info about size
    lseek(fd, sizeof(uint64_t), SEEK_CUR);

    if (read(fd, &out->page_id, sizeof(out->page_id)) != sizeof(out->page_id)) {
        fprintf(stderr, "Failed to read data in read_ph\n");
        return 1;
    }

    if (read(fd, &out->rows_count, sizeof(out->rows_count)) != sizeof(out->rows_count)) {
        fprintf(stderr, "Failed to read data in read_ph\n");
        return 2;
    }

    if (read(fd, &out->min_rid, sizeof(out->min_rid)) != sizeof(out->min_rid)) {
        fprintf(stderr, "Failed to read data in read_ph\n");
        return 3;
    }

    if (read(fd, &out->max_rid, sizeof(out->max_rid)) != sizeof(out->max_rid)) {
        fprintf(stderr, "Failed to read data in read_ph\n");
        return 4;
    }

    size_t pos = lseek(fd, 0, SEEK_CUR);
    out->file_size = lseek(fd, 0, SEEK_END) + 1;
    lseek(fd, pos, SEEK_SET);

    return 0;
}

int
skip_ph(int fd) {
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
static int
nb_size(const MetaTable* table) {
    return (table->columns_count + CHAR_BIT - 1) / CHAR_BIT;
}

/* Calculates an amount of null tokens in a null bitmap */
static int
nulls_count(unsigned char* nb, int len) {
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

static unsigned char*
build_nb(const MetaTable* table, const DataToken* tokens) {
    unsigned char* bitmap = calloc(nb_size(table), sizeof(char));
    if (!bitmap) {
        return NULL;
    }

    for (int i = 0; i < table->columns_count; i++) {
        if (tokens[i].type == DT_NULL) {
            int byteIndex = i / CHAR_BIT;
            int bitIndex = i % CHAR_BIT;
            bitmap[byteIndex] |= (1 << bitIndex);
        }
    }

    return bitmap;
}

static inline int
write_nb(const MetaTable* table, const DataToken* tokens, int fd) {
    unsigned char* nb = build_nb(table, tokens);
    int nbSize = nb_size(table);

    if (write(fd, nb, nbSize) != nbSize) {
        return 1;
    }

    return 0;
}

static inline bool
is_null(const unsigned char* nb, uint64_t size, size_t bit) {
    uint64_t byte_index = bit / CHAR_BIT;
    uint64_t bit_index = bit % size;

    return (nb[byte_index] >> bit_index) & 1;
}

uint64_t
dr_size_v(const MetaTable* table, const DataToken* tokens, int tokens_count) {
    int nbSize = nb_size(table);
    uint64_t totalSize = nbSize;
    for (int i = 0; i < tokens_count; i++) {
        totalSize += dt_size(&tokens[i]);
    }
    return totalSize;
}

uint64_t
dr_size(const MetaTable* table, const DataRow* row) {
    return dr_size_v(table, row->tokens, row->count);
}

static inline unsigned char*
read_nb(const MetaTable* table, int fd) {
    int nb__size = nb_size(table);
    unsigned char* bitmap = malloc(nb__size);

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

int
write_dr_flags(DataRowFlags flags, int fd) {
    if (write(fd, &flags, sizeof(DataRowFlags)) != sizeof(DataRowFlags)) {
        return 1;
    }

    return 0;
}

int
write_dr_v(const MetaTable* table,
           const DataToken* tokens,
           uint64_t rid,
           uint64_t count,
           unsigned char flags,
           int fd) {
    if (table == NULL || tokens == NULL) {
        return -1;
    }

    uint64_t rowSize = dr_size_v(table, tokens, count);

    if (write(fd, &rowSize, sizeof(rowSize)) != sizeof(rowSize)) {
        return 1;
    }
    if (write(fd, &rid, sizeof(rid)) != sizeof(rid)) {
        return 2;
    }
    if (write_dr_flags(flags, fd) != 0) {
        return 3;
    }
    if (write_nb(table, tokens, fd) != 0) {
        return 4;
    }

    for (int i = 0; i < count; i++) {
        if (write_dt(&tokens[i], fd) != 0) {
            return i + 5;
        }
    }

    return 0;
}

int
write_dr(const MetaTable* schema, const DataRow* row, int fd) {
    return write_dr_v(
        schema, row->tokens, row->row_id, row->count, row->flags, fd);
}

int
read_dr_flags(DataRowFlags* out, int fd) {
    if (read(fd, out, sizeof(DataRowFlags)) != sizeof(DataRowFlags)) {
        return 1;
    }
    return 0;
}

int
read_dr(const MetaTable* table,
        const char** column_names,
        size_t columns_count,
        DataRow* out,
        int fd) {
    uint64_t row_size;
    if (read(fd, &row_size, sizeof(row_size)) != sizeof(row_size)) {
        return 1;
    }

    uint64_t rid;
    if (read(fd, &rid, sizeof(rid)) != sizeof(rid)) {
        return 2;
    }

    DataRowFlags flags;
    if (read_dr_flags(&flags, fd) != 0) {
        return 3;
    }

    unsigned char* nb = read_nb(table, fd);
    if (!nb) {
        return 4;
    }

    int nb__size = nb_size(table);
    int nulls = nulls_count(nb, nb__size);

    bool* need_read = NULL;
    if (column_names && columns_count > 0) {
        need_read = malloc(table->columns_count * sizeof(bool));
        if (!need_read)
            return -10000;

        for (size_t i = 0; i < table->columns_count; ++i) {
            need_read[i] = false;
            for (size_t j = 0; j < columns_count; ++j) {
                if (strcmp(table->columns[i].name, column_names[j]) == 0) {
                    need_read[i] = true;
                    break;
                }
            }
        }
    }

    size_t final_columns_count = columns_count != 0 ? columns_count : table->columns_count;

    DataToken* tokens = malloc(final_columns_count * sizeof(DataToken));
    if (!tokens) {
        return 5;
    }

    for (int i = 0; i < table->columns_count; i++) {
        bool null = is_null(nb, nb__size, i);
        if (need_read && !need_read[i]) {
            if (!null) {
                DataToken tmp;
                read_dt(&tmp, fd);
            }
            continue;
        }

        if (null) {
            tokens[i] = make_token(DT_NULL, NULL, 0);
        } else {
            int res;
            if ((res = read_dt(&tokens[i], fd)) != 0) {
                return -i - 6;
            }
        }
    }

    if (need_read)
        free(need_read);

    out->tokens = tokens;
    out->count = final_columns_count;
    out->row_id = rid;
    out->flags = flags;
    return 0;
}

static inline uint64_t
mc_size(const MetaColumn* column) {
    return sizeof(column->id) + sizeof(uint64_t) + // length of a string
           strlen(column->name) + sizeof(column->data_type) + sizeof(column->flags);
}

int
write_mc(const MetaColumn* column, int fd) {
    ssize_t w;

    uint64_t size = mc_size(column);
    w = write(fd, &size, sizeof(size));
    if (w != sizeof(size)) {
        return 1;
    }

    w = write(fd, column->id, sizeof(column->id));
    if (w != sizeof(column->id)) {
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

int
read_mc(MetaColumn* column, uuid_t* table_id, int fd) {
    ssize_t r;
    uint64_t dcsize;

    if (table_id) {
        uuid_copy(column->table_id, *table_id);
    }

    r = read(fd, &dcsize, sizeof(dcsize));
    if (r != sizeof(dcsize)) {
        return 1;
    }

    r = read(fd, column->id, sizeof(column->id));
    if (r != sizeof(column->id)) {
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

int
write_mt(const MetaTable* table, int fd) {
    ssize_t w = 0;

    w = write(fd, table->id, sizeof(table->id));
    if (w != sizeof(table->id))
        return 1;

    w = write(fd, table->schema_id, sizeof(table->schema_id));
    if (w != sizeof(table->schema_id)) 
        return 2;

    uint64_t name_len = strlen(table->name);
    w = write(fd, &name_len, sizeof(name_len));
    if (w != sizeof(name_len))
        return 3;

    w = write(fd, table->name, name_len);
    if (w != name_len)
        return 4;

    w = write(fd, &table->has_pk, sizeof(table->has_pk));
    if (w != sizeof(table->has_pk))
        return 5;

    w = write(fd, &table->last_rid, sizeof(table->last_rid));
    if (w != sizeof(table->last_rid))
        return 6;

    if (table->has_pk) {
        w = write(fd, table->pk, sizeof(table->pk->id));
        if (w != sizeof(table->pk->id))
            return 7;
    }

    w = write(fd, &table->columns_count, sizeof(table->columns_count));
    if (w != sizeof(table->columns_count))
        return 8;

    for (size_t i = 0; i < table->columns_count; i++) {
        if (write_mc(&table->columns[i], fd) != 0) {
            return i + 9;
        }
    }

    return 0;
}

int
read_mt(MetaTable* out, int fd) {
    if (!out) {
        return -1;
    }

    int r = 0;
    if ((r = read(fd, out->id, sizeof(out->id))) != sizeof(out->id)) {
        return 1;
    }

    r = read(fd, out->schema_id, sizeof(out->schema_id));
    if (r != sizeof(out->schema_id)) 
        return 2;

    uint64_t name_len;
    if (read(fd, &name_len, sizeof(name_len)) != sizeof(name_len))
        return 3;

    out->name = malloc(name_len + 1);
    if (!out->name)
        return 4;

    if (read(fd, out->name, name_len) != (ssize_t)name_len) {
        free(out->name);
        return 5;
    }

    out->name[name_len] = '\0';

    if (read(fd, &out->has_pk, sizeof(out->has_pk)) != sizeof(out->has_pk)) {
        free(out->name);
        return 6;
    }

    if (read(fd, &out->last_rid, sizeof(out->last_rid)) != sizeof(out->last_rid)) {
        free(out->name);
        return 7;
    }

    if (out->has_pk) {
        if (read(fd, out->pk, sizeof(out->pk->id)) != sizeof(out->pk->id)) {
            free(out->name);
            return 8;
        }
    }

    if (read(fd, &out->columns_count, sizeof(out->columns_count)) != sizeof(out->columns_count)) {
        free(out->name);
        return 9;
    }

    out->columns = malloc(out->columns_count * sizeof(MetaColumn));
    if (!out->columns) {
        free(out->name);
        fprintf(stderr, "Failed to allocate memory for columns array in read_mt\n");
        return 9;
    }

    for (size_t i = 0; i < out->columns_count; i++) {
        if (read_mc(&out->columns[i], &out->id, fd) != 0) {
            for (size_t j = 0; j < i; j++) {
                free_col(&out->columns[j]);
            }

            free(out->columns);
            free(out->name);

            return i + 10;
        }
    }

    return 0;
}

int 
write_ms(const MetaSchema* schema, int fd) {
    if (!schema) {
        fprintf(stderr, "In write_ms: SCHEMA is NULL\n");
        return 1;
    }

    size_t w = 0;
    w = write(fd, schema->id, sizeof(uuid_t));
    if (w != sizeof(uuid_t)) {
        return 2;
    }

    size_t name_len = strlen(schema->name);
    w = write(fd, &name_len, sizeof(name_len));
    if (w != sizeof(name_len)) {
        return 3;
    }

    w = write(fd, schema->name, name_len);
    if (w != name_len) {
        return 4;
    }

    return 0;
}

int 
read_ms(MetaSchema* out, int fd) {
    if (!out) {
        fprintf(stderr, "In read_ms: OUT is NULL\n");
        return 1;
    }

    size_t r = 0;
    r = read(fd, out->id, sizeof(uuid_t));
    if (r != sizeof(uuid_t)) {
        return 2;
    }

    size_t name_len = 0;
    r = read(fd, &name_len, sizeof(name_len));
    if (r != sizeof(size_t)) {
        return 3;
    }
    
    out->name = malloc(name_len);
    if (!out->name) {
        fprintf(stderr, "In read_ms: failed to allocate memory for a name buffer\n");
        return 4;
    }

    r = read(fd, out->name, name_len);
    if (r != name_len) {
        return 5;
    }

    return 0;
}