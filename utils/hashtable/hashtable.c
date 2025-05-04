#include "hashtable.h"
#include <stdlib.h>
#include <string.h>

#define LOAD_FACTOR 0.75

/* djb2 hash function */
unsigned long hash(const char *str) {
    unsigned long hash = 5381;
    while (*str)
        hash = ((hash << 5) + hash) + (unsigned char)(*str++);
    return hash;
}

Entry *entry_create(const char *key, void *value) {
    Entry *entry = malloc(sizeof(Entry));
    entry->key = strdup(key);
    entry->value = value;
    entry->next = NULL;
    return entry;
}

HashMap *hm_create(size_t initial_capacity) {
    HashMap *map = malloc(sizeof(HashMap));
    map->capacity = initial_capacity;
    map->size = 0;
    map->buckets = calloc(initial_capacity, sizeof(Entry *));
    return map;
}

void hm_resize(HashMap *map);

void hm_put(HashMap *map, const char *key, void *value) {
    if ((double)map->size / map->capacity > LOAD_FACTOR)
        hm_resize(map);

    unsigned long index = hash(key) % map->capacity;
    Entry *curr = map->buckets[index];

    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            curr->value = value;
            return;
        }
        curr = curr->next;
    }

    Entry *new_entry = entry_create(key, value);
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
}

void *hm_get(HashMap *map, const char *key) {
    unsigned long index = hash(key) % map->capacity;
    Entry *curr = map->buckets[index];

    while (curr) {
        if (strcmp(curr->key, key) == 0)
            return curr->value;
        curr = curr->next;
    }
    return NULL;
}

void hm_resize(HashMap *map) {
    size_t new_capacity = map->capacity * 2;
    Entry **new_buckets = calloc(new_capacity, sizeof(Entry *));

    for (size_t i = 0; i < map->capacity; i++) {
        Entry *curr = map->buckets[i];
        while (curr) {
            Entry *next = curr->next;
            unsigned long index = hash(curr->key) % new_capacity;
            curr->next = new_buckets[index];
            new_buckets[index] = curr;
            curr = next;
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->capacity = new_capacity;
}

void hm_free(HashMap *map) {
    for (size_t i = 0; i < map->capacity; i++) {
        Entry *curr = map->buckets[i];
        while (curr) {
            Entry *next = curr->next;
            free(curr->key);
            free(curr);
            curr = next;
        }
    }
    free(map->buckets);
    free(map);
}
