#ifndef DATA_IO_H
#define DATA_IO_H

#include "data_token.h"
#include "data_page.h"
#include "data_table.h"

int write_dt_v(const char *bytes, DataType type, FILE *dest);
int write_dt_dv(const char *bytes, size_t size, DataType type, FILE *dest);
int write_dt(DataToken *token, FILE *dest);

int read_dt(DataToken *out, FILE *src);

int insert_dt(DataToken *token, size_t pos, FILE *file);

size_t dtok_size(const DataToken *token);
size_t read_length_prefix(FILE *file);

int write_ph(PageHeader *header, FILE *src);
int read_ph(PageHeader *out, FILE *src);

int write_dr(DataSchema *schema, DataRow *row, FILE *dest);
int write_dr_v(DataSchema *schema, DataToken **tokens, size_t count, unsigned char flags, FILE *dest);

int read_dr(DataSchema *schema, DataRow *out, FILE *src);

size_t dtype_size(DataType type);

#endif
