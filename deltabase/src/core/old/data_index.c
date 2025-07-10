#include "include/data_index.h"

#include <unistd.h>

void free_bp_key(BP_Key *key) {
    if (key->type == DT_STRING) 
        free(key->s);
}

BP_Node *bp_create_node(bool is_leaf) {
    BP_Node* node = calloc(1, sizeof(BP_Node));
    node->is_leaf = is_leaf;
    return node;
}

BP_Key bp_key_copy(BP_Key src) {
    if (src.type == DT_STRING) 
        return (BP_Key){ .type = DT_STRING, .s = strdup(src.s)};
    return src;
}

int bp_compare_keys(const BP_Key *left, const BP_Key *right) {
    if (left->type != right->type) return left->type - right->type;

    switch (left->type) {
        case DT_INTEGER: return (left->i > right->i) - (left->i < right->i);
        case DT_REAL: return (left->r > right->r) - (left->r < right->r);
        case DT_STRING: return strcmp(left->s, right->s);
        case DT_BOOL: return (left->b > right->b) - (left->b < right->b);
        case DT_CHAR: return (left->c > right->c) - (left->c < right->c);
    }

    return 0;
}

uint64_t *bp_search(BP_Node *root, const BP_Key *key) {
    int i = 0;
    while (i < root->keys_count && bp_compare_keys(key, &root->keys[i]) > 0) i++;

    if (root->is_leaf) {
        if (i < root->keys_count && bp_compare_keys(key, &root->keys[i]) == 0) 
            return &root->row_ids[i];
        return NULL;
    }

    return bp_search(root->children[i], key);
}

BP_SplitResult bp_insert_internal(BP_Node *node, BP_Key key, uint64_t row_id) {
    int i = 0;
    while (i < node->keys_count && bp_compare_keys(&key, &node->keys[i]) > 0) i++;

    if (node->is_leaf) {
        for (int j = node->keys_count; j > i; j--) {
            node->keys[j] = bp_key_copy(node->keys[j - 1]);
            node->row_ids[j] = node->row_ids[j - 1];
        }
        node->keys[i] = bp_key_copy(key);
        node->row_ids[i] = row_id;
        node->keys_count++;

        if (node->keys_count <= MAX_KEYS)
            return (BP_SplitResult){0};

        int mid = node->keys_count / 2;
        BP_Node *sibling = bp_create_node(true);
        sibling->keys_count = node->keys_count - mid;
        for (int j = 0; j < sibling->keys_count; j++) {
            sibling->keys[j] = node->keys[mid + j];
            sibling->row_ids[j] = node->row_ids[mid + j];
        }
        node->keys_count = mid;
        sibling->next_leaf = node->next_leaf;
        node->next_leaf = sibling;

        return (BP_SplitResult){
            .promoted_key = sibling->keys[0],
            .right = sibling,
            .has_split = true
        };
    } else {
        BP_SplitResult res = bp_insert_internal(node->children[i], key, row_id);
        if (!res.has_split) {
            return (BP_SplitResult){0};
        }

        for (int j = node->keys_count; j > i; j--) {
            node->keys[j] = bp_key_copy(node->keys[j - 1]);
            node->children[j + 1] = node->children[j];
        }
        node->keys[i] = res.promoted_key;
        node->children[i + 1] = res.right;
        node->keys_count++;

        if (node->keys_count <= MAX_KEYS) 
            return (BP_SplitResult){0};

        int mid = node->keys_count / 2;
        BP_Node *sibling = bp_create_node(false);
        sibling->keys_count = node->keys_count - mid - 1;
        
        for (int j = 0; j < sibling->keys_count; j++)
            sibling->keys[j] = node->keys[mid + 1 + j];

        for (int j = 0; j <= sibling->keys_count; j++)
            sibling->children[j] = node->children[mid + 1 + j];

        BP_Key promote = node->keys[mid];
        node->keys_count = mid;

        return (BP_SplitResult){
            .promoted_key = promote,
            .right = sibling,
            .has_split = true
        };
    }
}

void bp_insert(BP_Node **root_ref, BP_Key key, uint64_t row_id) {
    BP_Node *root = *root_ref;
    BP_SplitResult result = bp_insert_internal(root, key, row_id);
    if (result.has_split) {
        BP_Node *new_root = bp_create_node(false);
        new_root->keys[0] = result.promoted_key;
        new_root->children[0] = root;
        new_root->children[1] = result.right;
        new_root->keys_count = 1;
        *root_ref = new_root;
    }
}
