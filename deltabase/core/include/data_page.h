#ifndef DATA_PAGE_H
#define DATA_PAGE_H

#include <stddef.h>

/* Page header info */
typedef struct {
    size_t page_id;
    size_t rows_count;
} PageHeader;

#endif
