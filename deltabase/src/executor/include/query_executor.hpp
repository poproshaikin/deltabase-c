#ifndef QUERY_EXECUTOR_HPP
#define QUERY_EXECUTOR_HPP

#include <variant>
#include <memory>
#include <string>
#include "../../sql/include/parser.hpp"

extern "C" {
    #include "../../core/include/data.h"
}

namespace exe {
    class QueryExecutor {
        public: 
            QueryExecutor(std::string db_name);
            std::variant<std::unique_ptr<DataTable>, int> execute(const sql::AstNode& stmt);
        private:
            std::string db_name;
            std::unique_ptr<DataTable> execute_select(const sql::SelectStatement& stmt);
            int execute_insert(const sql::InsertStatement& stmt);
            int execute_update(const sql::UpdateStatement& stmt);
            int execute_delete(const sql::DeleteStatement& stmt);
            int execute_create_table(const sql::CreateTableStatement& stmt);
            int execute_create_database(const sql::CreateDbStatement& stmt);
    };
}

#endif
