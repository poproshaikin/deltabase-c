#ifndef SQL_VALUE_H
#define SQL_VALUE_H

#include "../../data/data-storage.h"

typedef enum ValueType {
    VT_INTEGER = 1,
    VT_REAL,
    VT_STRING,
    VT_BOOL,
    VT_TABLE,
} ValueType;

typedef struct TableColumn {
    char *name;
    int name_len;
    ValueType valueType;
} TableColumn;

typedef struct TableRow TableRow; // forward declaration

typedef struct Table {
    TableColumn *columns;
    int columns_count;
    TableRow *rows;
    int rows_count;
} Table;

typedef struct Value {
    ValueType type;
    union {
        int int_value;
        double real_value;
        struct {
            char *value;
            dulen_t len;
        } string;
        bool bool_value;
        Table table; 
    };
} Value;

struct TableRow {
    Value *values;
    dulen_t count;
};

#endif
