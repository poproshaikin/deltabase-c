#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>

typedef struct Entry {
    char *key;
    void *value;
    struct Entry *next;
} Entry;

typedef struct {
    Entry **buckets;
    size_t capacity;
    size_t size;
} HashMap;

HashMap *hm_create(size_t initial_capacity);
void hm_put(HashMap *map, const char *key, void *value);
void *hm_get(HashMap *map, const char *key);
void hm_free(HashMap *map);

#endif
