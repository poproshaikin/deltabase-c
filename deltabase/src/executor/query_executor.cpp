#include "include/query_executor.hpp"
#include "include/literals.hpp"
#include "../misc/include/utils.hpp"

#include <iostream>
#include <linux/limits.h>
#include <memory>
#include <stdexcept>
#include <uuid/uuid.h>
#include <variant>

#define HANDLE_ERROR_IF_NOT(FN_CALL, ERR_CODE, RET_CODE) if ((FN_CALL) != (ERR_CODE)) return (RET_CODE);

extern "C" {
    #include "../core/include/core.h"
    #include "../core/include/path_service.h"
    #include "../core/include/utils.h"
    #include "../core/include/data_io.h"
    #include "../core/include/data_filter.h"
}

const char* get_node_name(const sql::AstNode* node) {
    const sql::SqlToken& token = std::get<sql::SqlToken>(node->value);
    return token.value.data();
}

std::pair<void *, size_t> create_literal_value(const std::string& literal, DataType expected_type) {
    void* data = nullptr;
    size_t size = 0;

    if (expected_type == DT_INTEGER) {
        int32_t val = exe::literal_to_int(literal);
        size = sizeof(val);
        data = malloc(size);
        memcpy(data, &val, size);
    } else if (expected_type == DT_REAL) {
        double val = exe::literal_to_real(literal);
        size = sizeof(val);
        data = malloc(size);
        memcpy(data, &val, size);
    } else if (expected_type == DT_CHAR) {
        char val = exe::literal_to_char(literal);
        size = sizeof(val);
        data = malloc(size);
        memcpy(data, &val, size);
    } else if (expected_type == DT_BOOL) {
        bool val = exe::literal_to_bool(literal);
        size = sizeof(val);
        data = malloc(size);
        memcpy(data, &val, size);
    } else if (expected_type == DT_STRING) {
        const char* str = literal.c_str();
        size = literal.size();
        data = malloc(size);
        memcpy(data, str, size);
    }

    return { data, size };
}

DataToken* create_token_from_astnode(const sql::AstNode* node, DataType expected_type) {
    const sql::SqlToken& token = std::get<sql::SqlToken>(node->value);
    std::string literal = token.value;
    
    const auto value = create_literal_value(literal, expected_type);
    if (!value.first)
        throw std::runtime_error("Unsupported DataType in create_token_from_astnode");

    return make_token(expected_type, value.first, value.second);
}

DataRow* create_data_row_from_insert(const MetaTable* table, const sql::InsertStatement& insert) {
    if (!table) throw std::runtime_error("MetaTable is null");

    DataRow* row = new DataRow;
    row->flags = RF_NONE;
    row->count = table->columns_count;
    row->tokens = (DataToken**)calloc(row->count, sizeof(DataToken*));
    if (!row->tokens) {
        delete row;
        throw std::runtime_error("Failed to allocate tokens array");
    }

    for (uint64_t i = 0; i < table->columns_count; ++i) {
        const MetaColumn* col = table->columns[i];
        row->tokens[i] = nullptr;

        int insert_idx = -1;
        for (size_t j = 0; j < insert.columns.size(); ++j) {
            const char* ins_col_name = get_node_name(insert.columns[j].get());
            if (strcmp(ins_col_name, col->name) == 0) {
                insert_idx = (int)j;
                break;
            }
        }

        if (insert_idx == -1) {
            row->tokens[i] = make_token(DT_NULL, nullptr, 0);
        } else {
            const sql::AstNode* value_node = insert.values[insert_idx].get();
            row->tokens[i] = create_token_from_astnode(value_node, col->data_type);
        }
    }

    return row;
}

DataRowUpdate create_row_update(const MetaTable& table, const sql::UpdateStatement& query) {
    std::vector<DataRowUpdate> result;

    size_t assignments_count = query.assignments.size();

    uuid_t *indices = (uuid_t *)malloc(assignments_count * sizeof(uuid_t));
    void **values = (void **)malloc(assignments_count * sizeof(void *));

    for (size_t i = 0; i < assignments_count; i++) {
        const sql::BinaryExpr& assignment = std::get<sql::BinaryExpr>(query.assignments[i]->value);
        const sql::SqlToken& left = std::get<sql::SqlToken>(assignment.left->value);
        const sql::SqlToken& right = std::get<sql::SqlToken>(assignment.right->value);

        MetaColumn *column = find_column(left.value.data(), &table);
        if (!column) { // should not happen
            throw std::runtime_error("Column doesn't exist");
        }

        const auto value = create_literal_value(right.value, column->data_type);
    
        char uuid[37];
        uuid_unparse_lower((const unsigned char *)column->column_id, (char *)uuid);

        values[i] = malloc(value.second);
        memcpy(indices[i], column->column_id, sizeof(uuid_t));
        memcpy(values[i], value.first, value.second);
    }

    DataRowUpdate update = {
        .count = assignments_count,
        .column_indices = indices,
        .values = values
    };

    return update;
}

FilterOp parse_filter_op(sql::AstOperator op) {
    if (op == sql::AstOperator::EQ) return OP_EQ;
    if (op == sql::AstOperator::NEQ) return OP_NEQ;
    if (op == sql::AstOperator::LT) return OP_LT;
    if (op == sql::AstOperator::LTE) return OP_LTE;
    if (op == sql::AstOperator::GR) return OP_GT;
    if (op == sql::AstOperator::GRE) return OP_GTE;
    throw std::runtime_error("Unknown filter operator");
}

DataFilter create_filter(const sql::BinaryExpr& where, const MetaTable &table) {
    DataFilter filter = {};

    if (where.op == sql::AstOperator::AND || where.op == sql::AstOperator::OR) {
        const auto& left_expr = std::get<sql::BinaryExpr>(where.left->value);
        const auto& right_expr = std::get<sql::BinaryExpr>(where.right->value);

        DataFilter* left_filter = new DataFilter(create_filter(left_expr, table));
        DataFilter* right_filter = new DataFilter(create_filter(right_expr, table));

        filter.is_node = true;
        filter.data.node.left = left_filter;
        filter.data.node.right = right_filter;
        filter.data.node.op = (where.op == sql::AstOperator::AND) ? LOGIC_AND : LOGIC_OR;
        return filter;
    }

    const auto& left = std::get<sql::SqlToken>(where.left->value);
    const auto& right = std::get<sql::SqlToken>(where.right->value);
    const char *column_name = left.value.data();

    MetaColumn *column = find_column(column_name, &table);
    if (!column) {
        throw std::runtime_error("Column doesn't exist");
    }

    const auto value = create_literal_value(right.value, column->data_type);

    filter.is_node = false;
    memcpy(filter.data.condition.column_id, column->column_id, sizeof(uuid_t));
    filter.data.condition.op = parse_filter_op(where.op);
    filter.data.condition.type = column->data_type;
    filter.data.condition.value = value.first;
    return filter;
}

void file_deleter(FILE *f) { 
    if (f) fclose(f); 
}

using unique_file = std::unique_ptr<FILE, void(*)(FILE *)>;

namespace exe {
    QueryExecutor::QueryExecutor(std::string db_name) : _db_name(db_name) { }

    std::variant<std::unique_ptr<DataTable>, int> QueryExecutor::execute(const sql::AstNode& query) {
        if (std::holds_alternative<sql::SelectStatement>(query.value)) {
            return execute_select(std::get<sql::SelectStatement>(query.value));
        } 
        else if (std::holds_alternative<sql::InsertStatement>(query.value)) {
            return execute_insert(std::get<sql::InsertStatement>(query.value));
        }
        else if (std::holds_alternative<sql::UpdateStatement>(query.value)) {
            return execute_update(std::get<sql::UpdateStatement>(query.value));
        }
        else if (std::holds_alternative<sql::DeleteStatement>(query.value)) {
            return execute_delete(std::get<sql::DeleteStatement>(query.value));
        }

        throw std::runtime_error("Unsupported query");
    }

    std::unique_ptr<DataTable> QueryExecutor::execute_select(const sql::SelectStatement& query) {
        MetaTable schema;
        const sql::SqlToken table = std::get<sql::SqlToken>(query.table->value);

        if (get_table_schema(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        size_t columns_count = query.columns.size();

        char **column_names = (char**)std::malloc(columns_count * sizeof(char *));
        if (!column_names) {
            throw std::runtime_error("Failed to allocate memory in execute_query");
        }
        for (size_t i = 0; i < columns_count; i++) {
            column_names[i] = std::get<sql::SqlToken>(query.columns[i]->value).value.data();
        }

        std::unique_ptr<DataFilter> filter;
        if (query.where) 
            filter = std::make_unique<DataFilter>(create_filter(std::get<sql::BinaryExpr>(query.where->value), schema));

        DataTable result;
        if (seq_scan(_db_name.data(), table.value.data(), (const char **)column_names, columns_count, filter.get(), &result) != 0) {
            throw std::runtime_error("Failed to scan a table");
        }
        return std::make_unique<DataTable>(std::move(result)); 
    }

    int QueryExecutor::execute_insert(const sql::InsertStatement& query) {
        MetaTable schema;
        const sql::SqlToken table = std::get<sql::SqlToken>(query.table->value);

        if (get_table_schema(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        DataRow* row = create_data_row_from_insert(&schema, query);
        if (!row )
            throw std::runtime_error("Failed to create row from AST");

        if (insert_row(_db_name.data(), table.value.data(), row) != 0) {
            throw std::runtime_error("Failed to insert row");
        }

        return 1;
    }

    int QueryExecutor::execute_update(const sql::UpdateStatement& query) {
        MetaTable schema;
        const sql::SqlToken table = std::get<sql::SqlToken>(query.table->value);

        if (get_table_schema(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        DataRowUpdate update = create_row_update(schema, query);
        DataFilter filter = create_filter(std::get<sql::BinaryExpr>(query.where->value), schema);
        
        size_t rows_affected = 0;
        if (update_row_by_filter(_db_name.data(), table.value.data(), (const DataFilter *)&filter, &update, &rows_affected) != 0) {
            throw std::runtime_error("Failed to update row");
        }

        return rows_affected;
    }

    int QueryExecutor::execute_delete(const sql::DeleteStatement& query) {
        const sql::SqlToken table = std::get<sql::SqlToken>(query.table->value);

        MetaTable schema;
        if (get_table_schema(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        std::unique_ptr<DataFilter> filter = nullptr;
        if (query.where) {
            filter = std::make_unique<DataFilter>(create_filter(std::get<sql::BinaryExpr>(query.where->value), schema));
        }

        size_t rows_affected = 0;
        int rc = delete_row_by_filter(_db_name.data(), table.value.data(), filter.get(), &rows_affected);

        if (rc != 0) {
            throw std::runtime_error("Failed to delete rows by filter");
        }

        return rows_affected;
    }

    int QueryExecutor::execute_seq_scan(
        const std::string& db_name, 
        const std::string& table_name, 
        const std::vector<std::string>& column_names, 
        size_t columns_count, 
        const DataFilter& filter, 
        DataTable &out
    ) {
        std::string buffer(PATH_MAX, '\0');
        path_db_table_data(db_name.c_str(), table_name.c_str(), buffer.data(), PATH_MAX);

        MetaTable *schema = new (std::nothrow) MetaTable;
        if (!schema) {
            std::cerr << "Failed to allocate memory for meta table in execute_seq_scan" << std::endl;
            return 1;   
        }

        HANDLE_ERROR_IF_NOT(
            get_table_schema(db_name.c_str(), table_name.c_str(), schema), 
            0, 
            2);

        size_t pages_count;
        char **pages = get_dir_files(buffer.c_str(), &pages_count);
        if (!pages) {
            return 3;
        }

        std::vector<std::vector<std::unique_ptr<DataRow>>> page;
        page.reserve(pages_count);

        for (size_t i = 0; i < pages_count; i++) {
            unique_file file(fopen(pages[i], "r"), file_deleter);
            if (!file) {
                fprintf(stderr, "Failed to open file %s\n", pages[i]);
                // TODO               
            }

            int fd = fileno(file.get());
            PageHeader header;
            if (read_ph(&header, fd) != 0) {
                // TODO               
            }

            size_t count = 0;
            std::vector<std::unique_ptr<DataRow>> page;
            page.reserve(header.rows_count);

            for (size_t j = 0; j < header.rows_count; j++) {
                std::unique_ptr<DataRow> row = std::make_unique<DataRow>();

                char **col_names_ptr = string_vector_to_ptrs(column_names); 
 
                int res = 0;                
                if ((res = read_dr((const MetaTable *)schema, (const char **)col_names_ptr, columns_count, row.get(), fd)) != 0) {
                    fprintf(stderr, "Failed to read data row in full_scan: %i\n", res);
                }

                if (row->flags & RF_OBSOLETE) {
                    free_row(row.release());
                    continue;
                }

                if (filter && !row_satisfies_filter(schema, row.get(), filter)) {

                }
            }
        }
    }
}

