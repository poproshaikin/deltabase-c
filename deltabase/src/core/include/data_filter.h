#ifndef DATA_FILTER_H
#define DATA_FILTER_H

#include "data_token.h"
#include "stddef.h"
#include <stdbool.h>
#include <uuid/uuid.h>

typedef enum {
    OP_EQ = 1,  // ==
    OP_NEQ,     // !=
    OP_LT,      // <
    OP_LTE,     // <=
    OP_GT,      // >
    OP_GTE      // >=
} FilterOp;

typedef enum {
    LOGIC_AND = 1,
    LOGIC_OR
} LogicOp;

typedef struct DataFilter DataFilter;

typedef struct {
    uuid_t column_id;
    FilterOp op;
    DataType type;
    void *value;
} DataFilterCondition;

typedef struct {
    DataFilter *left;
    LogicOp op;
    DataFilter *right;
} DataFilterNode;

struct DataFilter {
    bool is_node;
    union {
        DataFilterCondition condition;
        DataFilterNode node;
    } data;
};

#endif
