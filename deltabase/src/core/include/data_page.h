#ifndef DATA_PAGE_H
#define DATA_PAGE_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <uuid/uuid.h>

static const size_t MAX_PAGE_SIZE = 8 * 1024;

/* Page header info */
typedef struct {
    uuid_t page_id;
    uint64_t rows_count;
    size_t file_size;
    uint64_t min_rid;
    uint64_t max_rid;
} PageHeader;

#endif
