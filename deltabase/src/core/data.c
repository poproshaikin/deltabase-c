#include "include/data.h"
#include "include/ll_io.h"
#include "include/paths.h"
#include "include/utils.h"
#include <linux/limits.h>

DataToken
make_token(DataType type, const void* data, size_t size) {
    DataToken token = {
        .type = type,
        .size = size,
        .bytes = (char*)malloc(size)
    };
    memcpy(token.bytes, data, size);
    return token;
}

void
free_token(DataToken* token) {
    if (token) {
        free(token->bytes);
    }
}

DataToken
copy_token(DataToken old) {
    DataToken new_token = {
        .type = old.type,
        .size = old.size,
        .bytes = malloc(old.size)
    };
    memcpy(new_token.bytes, old.bytes, old.size);
    return new_token;
}

void
free_tokens(DataToken* tokens, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free_token(&tokens[i]);
    }
}

void
free_row(DataRow* row) {
    free(row->tokens);
}

void
free_col(MetaColumn* column) {
    free(column->name);
}

bool
apply_filter(DataFilterCondition condition, const void* db_value, const DataType db_type) {
    if (db_type == DT_UNDEFINED || condition.type == DT_UNDEFINED) {
        fprintf(stderr, "In apply_filter: one of passed comparing types was undefined\n");
        return false;
    }

    char col_id[37];
    if (condition.type == DT_NULL || db_type == DT_NULL) {
        return false;
    }

    if (condition.type != db_type) {
        fprintf(stderr, "apply_filter: type in condition isn't the same as type in database\n");
        // TODO: implicit casting for compatible types (e.g. INT to REAL etc)
        return false;
    }

    switch (condition.type) {
    case DT_UNDEFINED:
        return false;
    case DT_INTEGER: {
        int32_t a = *(const int32_t*)condition.value;
        int32_t b = *(const int32_t*)db_value;

        switch (condition.op) {
        case OP_EQ:
            return a == b;
        case OP_NEQ:
            return a != b;
        case OP_LT:
            return a < b;
        case OP_LTE:
            return a <= b;
        case OP_GT:
            return a > b;
        case OP_GTE:
            return a >= b;
        }
        break;
    }
    case DT_REAL: {
        double a = *(const double*)condition.value;
        double b = *(const double*)db_value;

        switch (condition.op) {
        case OP_EQ:
            return a == b;
        case OP_NEQ:
            return a != b;
        case OP_LT:
            return a < b;
        case OP_LTE:
            return a <= b;
        case OP_GT:
            return a > b;
        case OP_GTE:
            return a >= b;
        }
        break;
    }
    case DT_STRING: {
        const char* a = (const char*)condition.value;
        const char* b = (const char*)db_value;

        int cmp = strcmp(a, b);
        switch (condition.op) {
        case OP_EQ:
            return cmp == 0;
        case OP_NEQ:
            return cmp != 0;
        case OP_LT:
            return cmp < 0;
        case OP_LTE:
            return cmp <= 0;
        case OP_GT:
            return cmp > 0;
        case OP_GTE:
            return cmp >= 0;
        }
        break;
    }
    case DT_CHAR: {
        char a = *(const char*)condition.value;
        char b = *(const char*)db_value;
        switch (condition.op) {
        case OP_EQ:
            return a == b;
        case OP_NEQ:
            return a != b;
        case OP_LT:
            return a < b;
        case OP_LTE:
            return a <= b;
        case OP_GT:
            return a > b;
        case OP_GTE:
            return a >= b;
        }
        break;
    }
    case DT_BOOL: {
        bool a = *(const bool*)condition.value;
        bool b = *(const bool*)db_value;

        switch (condition.op) {
        case OP_EQ:
            return a == b;
        case OP_NEQ:
            return a != b;
        default:
            return false;
        }
    }
    case DT_NULL:
        break;
    }

    fprintf(stderr, "Unsupported comparison: %i to %i\n", condition.type, db_type);

    return false;
}

bool
row_satisfies_filter(const MetaTable* schema, const DataRow* row, const DataFilter* filter) {
    if (schema->columns_count != row->count) {
        fprintf(stderr,
                "row_satisfies_filter: Count of columns in schema wasn't equal to count of "
                "tokens in row\n");
        return false;
    }

    if (!filter->is_node) {
        ssize_t col_index = get_table_column_index(filter->data.condition.column_id, schema);

        if (col_index < 0 || col_index >= row->count) { // wtf
            fprintf(stderr, "Column with ID provided in filter was not found in table\n");
            return false;
        }

        return apply_filter(
            filter->data.condition, row->tokens[col_index].bytes, row->tokens[col_index].type);
    } else {
        switch (filter->data.node.op) {
        case LOGIC_AND:
            return row_satisfies_filter(schema, row, filter->data.node.left) &&
                   row_satisfies_filter(schema, row, filter->data.node.right);
        case LOGIC_OR:
            return row_satisfies_filter(schema, row, filter->data.node.left) ||
                   row_satisfies_filter(schema, row, filter->data.node.right);
        default:
            fprintf(stderr, "Unsupported logic operator: %i\n", filter->data.node.op);
            return false;
        }
    }
}

int
create_page(const char* db_name,
            const char* table_name,
            PageHeader* out_page,
            char** out_path) {
    if (!out_path) {
        fprintf(stderr, "Passed NULL to OUT_PAGE \n");
        return 4;
    }

    PageHeader local_header;
    PageHeader* header = out_page ? out_page : &local_header;

    uuid_generate_time(header->page_id);
    header->rows_count = 0;

    char file_path[PATH_MAX];
    path_db_table_page(db_name, table_name, header->page_id, file_path, PATH_MAX);

    FILE* file = fopen(file_path, "w+");
    if (!file) {
        fprintf(stderr, "Failed to create page file %s\n", header->page_id);
        return 1;
    }

    if (write_ph(header, fileno(file)) != 0) {
        return 2;
    }

    size_t len = strlen(file_path);
    char* path = calloc(len + 1, sizeof(char));

    if (!path) {
        fclose(file);
        return 3;
    }

    memcpy(path, file_path, len);
    path[len] = '\0';

    if (out_path) {
        *out_path = path;
    }

    fclose(file);
    return 0;
}

PagePaths
get_pages(const char* db_name, const char* table_name) {
    PagePaths result = {0};
    
    char dir_path[PATH_MAX];
    path_db_table_data(db_name, table_name, dir_path, PATH_MAX);

    result.paths = get_dir_files(dir_path, &result.count);
    
    return result;
}