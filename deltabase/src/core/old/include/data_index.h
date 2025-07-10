#ifndef DATA_INDEX_H
#define DATA_INDEX_H

#include "data_token.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_KEYS 3
#define BPLUS_ORDER 4

typedef struct BP_Key {
    DataType type;
    union {
        int i;
        double r;
        char *s;
        bool b;
        char c;
    };
} BP_Key;

typedef struct BP_Node {
    bool is_leaf;
    int keys_count;
    BP_Key keys[MAX_KEYS];
    struct BP_Node *children[MAX_KEYS + 1]; // internals
    uint64_t row_ids[MAX_KEYS]; // leafs
    struct BP_Node *next_leaf;
} BP_Node;

void free_bp_key(BP_Key *key);
BP_Node *bp_create_node(bool is_leaf);

int bp_compare_keys(const BP_Key *left, const BP_Key *right);
uint64_t *bp_search(BP_Node *root, const BP_Key *key);

typedef struct BP_SplitResult {
    BP_Key promoted_key;
    BP_Node *right;
    bool has_split;
} BP_SplitResult;

BP_SplitResult bp_insert_internal(BP_Node *node, BP_Key key, uint64_t row_id);
void bp_insert(BP_Node **root_ref, BP_Key key, uint64_t row_id);

#endif
