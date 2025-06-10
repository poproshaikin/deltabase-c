#ifndef DATA_IO_H
#define DATA_IO_H

#include "data_token.h"
#include "data_page.h"
#include "data_table.h"

int write_dt_v(const char *bytes, DataType type, int fd);
int write_dt_dv(const char *bytes, size_t size, DataType type, int fd);
int write_dt(const DataToken *token, int fd);

int read_dt(DataToken *out, int fd);

int insert_dt(const DataToken *token, size_t pos, int fd);

size_t dtok_size(const DataToken *token);
size_t read_length_prefix(int fd);

int write_ph(const PageHeader *header, int fd);
int read_ph(PageHeader *out, int fd);

int write_dr(const DataSchema *schema, const DataRow *row, int fd);
int write_dr_v(const DataSchema *schema, const DataToken **tokens, size_t count, unsigned char flags, int fd);

int read_dr(const DataSchema *schema, DataRow *out, int fd);

size_t dtype_size(DataType type);

#endif
