#ifndef CORE_LOWLEVEL_IO_H
#define CORE_LOWLEVEL_IO_H

#include "meta.h"
#include "data.h"
#include "utils.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

int write_dt_v(const char *bytes, DataType type, int fd);
int write_dt_dv(const char *bytes, uint64_t size, DataType type, int fd);
int write_dt(const DataToken *token, int fd);

int read_dt(DataToken *out, int fd);

int insert_dt(const DataToken *token, uint64_t pos, int fd);

uint64_t dtok_size(const DataToken *token);
int read_length_prefix(uint64_t *out, int fd);
int read_uuid(char **out_buffer_ptr, int fd);

int write_ph(const PageHeader *header, int fd);
int read_ph(PageHeader *out, int fd);
int skip_ph(int fd);

int write_dr(const MetaTable *schema, const DataRow *row, int fd);
int write_dr_v(const MetaTable *schema, const DataToken **tokens, uint64_t rid, uint64_t count, unsigned char flags, int fd);
int write_dr_flags(DataRowFlags flags, int fd);

int read_dr(const MetaTable *schema, const char **column_names, size_t columns_count, DataRow *out, int fd);

uint64_t dtype_size(DataType type);

int write_mt(const MetaTable *schema, int fd);
int read_mt(MetaTable *out, int fd);

int write_mc(const MetaColumn *column, int fd);
int read_mc(MetaColumn *column, int fd);

#endif // CORE_LOWLEVEL_IO_H