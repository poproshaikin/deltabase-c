#include "include/query_executor.hpp"
#include "../converter/include/converter.hpp"
#include "../converter/include/statement_converter.hpp"

#include "../misc/include/utils.hpp"
#include "../meta/include/meta_factory.hpp"

#include <linux/limits.h>
#include <memory>
#include <stdexcept>
#include <uuid/uuid.h>
#include <variant>

#define HANDLE_ERROR_IF_NOT(FN_CALL, ERR_CODE, RET_CODE) if ((FN_CALL) != (ERR_CODE)) return (RET_CODE);

extern "C" {
    #include "../core/include/core.h"
    #include "../core/include/meta.h"
    #include "../core/include/data.h"
}

namespace {
    const char* get_node_name(const sql::AstNode* node) {
        const sql::SqlToken& token = std::get<sql::SqlToken>(node->value);
        return token.value.data();
    }



    void file_deleter(FILE *f) {
        if (f)
            fclose(f);
    }

    using unique_file = std::unique_ptr<FILE, void (*)(FILE *)>;

} // anonymous namespace

namespace exe {
    QueryExecutor::QueryExecutor(std::string db_name) : db_name(db_name) { }

    std::variant<std::unique_ptr<DataTable>, int> QueryExecutor::execute(const sql::AstNode& stmt) {
        if (std::holds_alternative<sql::SelectStatement>(stmt.value)) {
            return execute_select(std::get<sql::SelectStatement>(stmt.value));
        } 
        else if (std::holds_alternative<sql::InsertStatement>(stmt.value)) {
            return execute_insert(std::get<sql::InsertStatement>(stmt.value));
        }
        else if (std::holds_alternative<sql::UpdateStatement>(stmt.value)) {
            return execute_update(std::get<sql::UpdateStatement>(stmt.value));
        }
        else if (std::holds_alternative<sql::DeleteStatement>(stmt.value)) {
            return execute_delete(std::get<sql::DeleteStatement>(stmt.value));
        } else if (std::holds_alternative<sql::CreateTableStatement>(stmt.value)) {
            return execute_create_table(std::get<sql::CreateTableStatement>(stmt.value));
        }

        throw std::runtime_error("Unsupported query");
    }

    std::unique_ptr<DataTable> QueryExecutor::execute_select(const sql::SelectStatement& stmt) {
        MetaTable *schema = new MetaTable;
        const sql::SqlToken& table = stmt.table;

        if (get_table(this->db_name.data(), table.value.data(), schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        size_t columns_count = stmt.columns.size();

        char **column_names = (char**)std::malloc(columns_count * sizeof(char *));
        if (!column_names) {
            throw std::runtime_error("Failed to allocate memory in execute_query");
        }
        for (size_t i = 0; i < columns_count; i++) {
            column_names[i] = const_cast<char*>(stmt.columns[i].value.data());
        }

        std::unique_ptr<DataFilter> filter;
        if (stmt.where) 
            filter = std::make_unique<DataFilter>(converter::convert_binary_to_filter(std::get<sql::BinaryExpr>(stmt.where->value), *schema));

        DataTable result;
        if (seq_scan(this->db_name.data(), table.value.data(), (const char **)column_names, columns_count, filter.get(), &result) != 0) {
            throw std::runtime_error("Failed to scan a table");
        }
        result.scheme = schema;

        return std::make_unique<DataTable>(std::move(result)); 
    }

    int QueryExecutor::execute_insert(const sql::InsertStatement& stmt) {
        MetaTable schema;
        const sql::SqlToken& table = stmt.table;

        if (get_table(this->db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        DataRow row = converter::convert_insert_to_data_row(schema, stmt);
        if (insert_row(this->db_name.data(), table.value.data(), &row) != 0) {
            throw std::runtime_error("Failed to insert row");
        }

        meta::cleanup_meta_table(schema);

        return 1;
    }

    int QueryExecutor::execute_update(const sql::UpdateStatement& stmt) {
        MetaTable schema;
        const sql::SqlToken& table = stmt.table;

        if (get_table(this->db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        DataRowUpdate update = converter::create_row_update(schema, stmt);
        DataFilter filter = converter::convert_binary_to_filter(std::get<sql::BinaryExpr>(stmt.where->value), schema);
        
        size_t rows_affected = 0;
        if (update_rows_by_filter(this->db_name.data(), table.value.data(), (const DataFilter *)&filter, &update, &rows_affected) != 0) {
            throw std::runtime_error("Failed to update row");
        }

        meta::cleanup_meta_table(schema);

        return rows_affected;
    }

    int QueryExecutor::execute_delete(const sql::DeleteStatement& stmt) {
        const sql::SqlToken& table = stmt.table;

        MetaTable schema;
        if (get_table(this->db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        std::unique_ptr<DataFilter> filter = nullptr;
        if (stmt.where) {
            filter = std::make_unique<DataFilter>(converter::convert_binary_to_filter(std::get<sql::BinaryExpr>(stmt.where->value), schema));
        }

        size_t rows_affected = 0;
        int rc = delete_rows_by_filter(this->db_name.data(), table.value.data(), filter.get(), &rows_affected);

        if (rc != 0) {
            throw std::runtime_error("Failed to delete rows by filter");
        }

        meta::cleanup_meta_table(schema);

        return rows_affected;
    }

    int QueryExecutor::execute_create_table(const sql::CreateTableStatement& stmt) {
        MetaTable table = converter::convert_create_table_to_mt(stmt);

        if (create_table(this->db_name.data(), &table) != 0) {
            throw std::runtime_error("Failed to create table");
        }

        meta::cleanup_meta_table(table);

        return 0;
    }

    int QueryExecutor::execute_create_database(const sql::CreateDbStatement &stmt) {
        if (!create_database(stmt.name.value.c_str())) {
            throw std::runtime_error("Failed to create database " + stmt.name.value);
        }

        return 0;
    }
} // namespace exe
