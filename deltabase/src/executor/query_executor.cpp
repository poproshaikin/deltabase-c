#include "include/query_executor.hpp"
#include "../converter/include/converter.hpp"
#include "../converter/include/statement_converter.hpp"
#include "../catalog/include/meta_registry.hpp"
#include "../catalog/include/data_object.hpp"
#include "../catalog/include/models.hpp"
#include "../catalog/include/information_schema.hpp"

#include <linux/limits.h>
#include <memory>
#include <stdexcept>
#include <uuid/uuid.h>

extern "C" {
#include "../core/include/core.h"
#include "../core/include/data.h"
#include "../core/include/meta.h"
}

namespace {
    const char*
    get_node_name(const sql::AstNode* node) {
        const sql::SqlToken& token = std::get<sql::SqlToken>(node->value);
        return token.value.data();
    }

    void
    file_deleter(FILE* f) {
        if (f)
            fclose(f);
    }

    using unique_file = std::unique_ptr<FILE, void (*)(FILE*)>;
} // anonymous namespace

namespace exe {
    IQueryExecutor::~IQueryExecutor() {
    }

    void
    DatabaseExecutor::set_db_name(std::string db_name) {
        this->db_name = db_name;
    }

    IntOrDataTable
    DatabaseExecutor::execute(const sql::AstNode& node) {
        if (node.type == sql::AstNodeType::SELECT) {
            return execute_select(std::get<sql::SelectStatement>(node.value));
        }

        if (node.type == sql::AstNodeType::INSERT) {
            return execute_insert(std::get<sql::InsertStatement>(node.value));
        }

        if (node.type == sql::AstNodeType::UPDATE) {
            return execute_update(std::get<sql::UpdateStatement>(node.value));
        }

        if (node.type == sql::AstNodeType::DELETE) {
            return execute_delete(std::get<sql::DeleteStatement>(node.value));
        }

        if (node.type == sql::AstNodeType::CREATE_TABLE) {
            return execute_create_table(std::get<sql::CreateTableStatement>(node.value));
        }

        throw std::runtime_error("Unsupported query");
    }

    std::unique_ptr<catalog::CppDataTable>
    DatabaseExecutor::execute_select(const sql::SelectStatement& stmt) {
        auto cpp_table = this->registry.get_table(stmt.table.table_name);
        MetaTable c_table = cpp_table.create_meta_table();

        const sql::SqlToken& table_name = stmt.table.table_name;

        size_t columns_count = stmt.columns.size();

        char** column_names = (char**)std::malloc(columns_count * sizeof(char*));
        if (!column_names) {
            throw std::runtime_error("Failed to allocate memory in execute_query");
        }
        for (size_t i = 0; i < columns_count; i++) {
            column_names[i] = const_cast<char*>(stmt.columns[i].value.data());
        }

        std::unique_ptr<DataFilter> filter;
        if (stmt.where)
            filter = std::make_unique<DataFilter>(converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), c_table));


        DataTable result;
        if (seq_scan(this->db_name.data(),
                     &c_table,
                     (const char**)column_names,
                     columns_count,
                     filter.get(),
                     &result) != 0) {
            throw std::runtime_error("Failed to scan a table");
        }

        result.schema = c_table;

        return std::make_unique<catalog::CppDataTable>(std::move(result));
    }

    int
    DatabaseExecutor::execute_insert(const sql::InsertStatement& stmt) {
        auto table = this->registry.get_table(stmt.table.table_name.value);
        auto c_table = table.create_meta_table();

        DataRow row = converter::convert_insert_to_data_row(c_table, stmt);
        if (insert_row(this->db_name.c_str(), &c_table, &row) != 0) {
            throw std::runtime_error("Failed to insert row");
        }

        catalog::models::cleanup_meta_table(c_table);

        return 1;
    }

    int
    DatabaseExecutor::execute_update(const sql::UpdateStatement& stmt) {
        auto table = this->registry.get_table(stmt.table.table_name.value);
        auto c_table = table.create_meta_table();

        DataRowUpdate update = converter::create_row_update(c_table, stmt);
        DataFilter filter = converter::convert_binary_to_filter(
            std::get<sql::BinaryExpr>(stmt.where->value), c_table);

        size_t rows_affected = 0;
        if (update_rows_by_filter(this->db_name.data(),
                                  table.get_name().c_str(),
                                  (const DataFilter*)&filter,
                                  &update,
                                  &rows_affected) != 0) {
            throw std::runtime_error("Failed to update row");
        }

        catalog::models::cleanup_meta_table(c_table);

        return rows_affected;
    }

    int
    DatabaseExecutor::execute_delete(const sql::DeleteStatement& stmt) {
        auto table = this->registry.get_table(stmt.table.table_name.value);
        auto c_table = table.create_meta_table();

        std::unique_ptr<DataFilter> filter = nullptr;
        if (stmt.where) {
            filter = std::make_unique<DataFilter>(converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), c_table));
        }

        size_t rows_affected = 0;
        int rc = delete_rows_by_filter(
            this->db_name.c_str(), table.get_name().c_str(), filter.get(), &rows_affected);

        if (rc != 0) {
            throw std::runtime_error("Failed to delete rows by filter");
        }

        catalog::models::cleanup_meta_table(c_table);
        return rows_affected;
    }

    int
    DatabaseExecutor::execute_create_table(const sql::CreateTableStatement& stmt) {
        MetaTable table = converter::convert_create_table_to_mt(stmt);

        if (create_table(this->db_name.data(), &table) != 0) {
            throw std::runtime_error("Failed to create table");
        }

        catalog::models::cleanup_meta_table(table);

        return 0;
    }

    void
    AdminExecutor::set_db_name(std::string db_name) {
        this->db_name = db_name;
    }

    IntOrDataTable
    AdminExecutor::execute(const sql::AstNode& node) {
        if (node.type == sql::AstNodeType::CREATE_DATABASE) {
            return this->execute_create_database(std::get<sql::CreateDbStatement>(node.value));
        }

        throw std::runtime_error("Unsupported query");
    }

    int
    AdminExecutor::execute_create_database(const sql::CreateDbStatement& stmt) {
        if (create_database(stmt.name.value.c_str()) != 0) {
            throw std::runtime_error("Failed to create database " + stmt.name.value);
        }

        return 0;
    }

    IntOrDataTable
    VirtualExecutor::execute(const sql::AstNode& node) {
        if (node.type == sql::AstNodeType::SELECT) {
            const sql::SelectStatement& stmt = std::get<sql::SelectStatement>(node.value);

            if (stmt.table.table_name.value == "tables") {
                return this->execute_information_schema_tables();
            }
            if (stmt.table.table_name.value == "columns") {
                return this->execute_information_schema_columns();
            }
        }

        throw std::runtime_error("Unsupported query for VirtualExecutor");
    }

    std::unique_ptr<catalog::CppDataTable>
    VirtualExecutor::execute_information_schema_tables() {
        catalog::CppDataTable table =
            catalog::information_schema::get_tables_data(this->registry.get_tables());

        return std::make_unique<catalog::CppDataTable>(std::move(table));
    }   

    std::unique_ptr<catalog::CppDataTable>
    VirtualExecutor::execute_information_schema_columns() {
        catalog::CppDataTable table =
            catalog::information_schema::get_columns_data(this->registry.get_columns());

        return std::make_unique<catalog::CppDataTable>(std::move(table));
    }
} // namespace exe
