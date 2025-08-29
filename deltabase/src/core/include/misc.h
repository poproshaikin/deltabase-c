#ifndef CORE_MISC_H
#define CORE_MISC_H

#include <stddef.h>
#include <uuid/uuid.h>

typedef struct {
    size_t count;
    uuid_t* column_indices;
    void** values;
} DataRowUpdate;

#endif