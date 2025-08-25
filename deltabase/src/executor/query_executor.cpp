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
        } else if (std::holds_alternative<sql::CreateTableStatement>(query.value)) {
            return execute_create_table(std::get<sql::CreateTableStatement>(query.value));
        }

        throw std::runtime_error("Unsupported query");
    }

    std::unique_ptr<DataTable> QueryExecutor::execute_select(const sql::SelectStatement& query) {
        MetaTable *schema = new MetaTable;
        const sql::SqlToken& table = query.table;

        if (get_table(this->db_name.data(), table.value.data(), schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        size_t columns_count = query.columns.size();

        char **column_names = (char**)std::malloc(columns_count * sizeof(char *));
        if (!column_names) {
            throw std::runtime_error("Failed to allocate memory in execute_query");
        }
        for (size_t i = 0; i < columns_count; i++) {
            column_names[i] = const_cast<char*>(query.columns[i].value.data());
        }

        std::unique_ptr<DataFilter> filter;
        if (query.where) 
            filter = std::make_unique<DataFilter>(converter::convert_binary_to_filter(std::get<sql::BinaryExpr>(query.where->value), *schema));

        DataTable result;
        if (seq_scan(this->db_name.data(), table.value.data(), (const char **)column_names, columns_count, filter.get(), &result) != 0) {
            throw std::runtime_error("Failed to scan a table");
        }
        result.scheme = schema;

        return std::make_unique<DataTable>(std::move(result)); 
    }

    int QueryExecutor::execute_insert(const sql::InsertStatement& query) {
        MetaTable schema;
        const sql::SqlToken& table = query.table;

        if (get_table(this->db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        DataRow row = converter::convert_insert_to_data_row(schema, query);
        if (insert_row(this->db_name.data(), table.value.data(), &row) != 0) {
            throw std::runtime_error("Failed to insert row");
        }

        meta::cleanup_meta_table(schema);

        return 1;
    }

    int QueryExecutor::execute_update(const sql::UpdateStatement& query) {
        MetaTable schema;
        const sql::SqlToken& table = query.table;

        if (get_table(this->db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        DataRowUpdate update = converter::create_row_update(schema, query);
        DataFilter filter = converter::convert_binary_to_filter(std::get<sql::BinaryExpr>(query.where->value), schema);
        
        size_t rows_affected = 0;
        if (update_rows_by_filter(this->db_name.data(), table.value.data(), (const DataFilter *)&filter, &update, &rows_affected) != 0) {
            throw std::runtime_error("Failed to update row");
        }

        meta::cleanup_meta_table(schema);

        return rows_affected;
    }

    int QueryExecutor::execute_delete(const sql::DeleteStatement& query) {
        const sql::SqlToken& table = query.table;

        MetaTable schema;
        if (get_table(this->db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error(std::string("Failed to get table schema: ") + table.value);
        }

        std::unique_ptr<DataFilter> filter = nullptr;
        if (query.where) {
            filter = std::make_unique<DataFilter>(converter::convert_binary_to_filter(std::get<sql::BinaryExpr>(query.where->value), schema));
        }

        size_t rows_affected = 0;
        int rc = delete_rows_by_filter(this->db_name.data(), table.value.data(), filter.get(), &rows_affected);

        if (rc != 0) {
            throw std::runtime_error("Failed to delete rows by filter");
        }

        meta::cleanup_meta_table(schema);

        return rows_affected;
    }

    int QueryExecutor::execute_create_table(const sql::CreateTableStatement& query) {
        MetaTable table = converter::convert_create_table_to_mt(query);

        if (create_table(this->db_name.data(), &table) != 0) {
            throw std::runtime_error("Failed to create table");
        }

        meta::cleanup_meta_table(table);

        return 0;
    }

    // int QueryExecutor::execute_seq_scan(
    //     const std::string& db_name, 
    //     const std::string& table_name, 
    //     const std::vector<std::string>& column_names, 
    //     size_t columns_count, 
    //     const DataFilter& filter, 
    //     DataTable &out
    // ) {
    //     std::string buffer(PATH_MAX, '\0');
    //     path_db_table_data(db_name.c_str(), table_name.c_str(), buffer.data(), PATH_MAX);

    //     MetaTable *schema = new (std::nothrow) MetaTable;
    //     if (!schema) {
    //         std::cerr << "Failed to allocate memory for meta table in execute_seq_scan" << std::endl;
    //         return 1;   
    //     }

    //     HANDLE_ERROR_IF_NOT(
    //         get_table(db_name.c_str(), table_name.c_str(), schema), 
    //         0, 
    //         2);

    //     size_t pages_count;
    //     char **pages = get_dir_files(buffer.c_str(), &pages_count);
    //     if (!pages) {
    //         return 3;
    //     }

    //     std::vector<std::vector<std::unique_ptr<DataRow>>> page;
    //     page.reserve(pages_count);

    //     for (size_t i = 0; i < pages_count; i++) {
    //         unique_file file(fopen(pages[i], "r"), file_deleter);
    //         if (!file) {
    //             fprintf(stderr, "Failed to open file %s\n", pages[i]);
    //             // TODO               
    //         }

    //         int fd = fileno(file.get());
    //         PageHeader header;
    //         if (read_ph(&header, fd) != 0) {
    //             // TODO               
    //         }

    //         size_t count = 0;
    //         std::vector<std::unique_ptr<DataRow>> page;
    //         page.reserve(header.rows_count);

    //         for (size_t j = 0; j < header.rows_count; j++) {
    //             std::unique_ptr<DataRow> row = std::make_unique<DataRow>();

    //             char **col_names_ptr = string_vector_to_ptrs(column_names); 
 
    //             int res = 0;                
    //             if ((res = read_dr((const MetaTable *)schema, (const char **)col_names_ptr, columns_count, row.get(), fd)) != 0) {
    //                 fprintf(stderr, "Failed to read data row in full_scan: %i\n", res);
    //             }

    //             if (row->flags & RF_OBSOLETE) {
    //                 free_row(row.release());
    //                 continue;
    //             }

    //             if (!row_satisfies_filter(schema, row.get(), &filter)) {

    //             }
    //         }
    //     }
    // }
} // namespace exe

