#ifndef CORE_INDEX_H
#define CORE_INDEX_H

#include "data.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_KEYS 3

// Simple index key that can hold any data type
typedef struct IndexKey {
    DataType type;
    union {
        int i;
        double r;
        char *s;
        bool b;
        char c;
    } value;
} IndexKey;

// B+ Tree node
typedef struct IndexNode {
    bool is_leaf;
    int keys_count;
    IndexKey keys[MAX_KEYS];
    struct IndexNode *children[MAX_KEYS + 1]; // for internal nodes
    uint64_t row_ids[MAX_KEYS]; // for leaf nodes
    struct IndexNode *next_leaf; // leaf linking
} IndexNode;

// Basic operations
IndexNode *create_index_node(bool is_leaf);
int compare_index_keys(const IndexKey *left, const IndexKey *right);
uint64_t *search_index(IndexNode *root, const IndexKey *key);
void insert_into_index(IndexNode **root, const IndexKey *key, uint64_t row_id);

#endif // CORE_INDEX_H