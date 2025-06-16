#pragma once

#include "semantic_analyzer.hpp"
#include <chrono>
#include <string>

extern "C" {
    #include "../../core/include/data_table.h"
}

namespace exe {
    class QueryExecutor {
        public: 
            QueryExecutor(std::string db_name);
            std::variant<std::unique_ptr<DataTable>, int> execute(const sql::AstNode& query);
        private:
            std::string _db_name;
            std::unique_ptr<DataTable> execute_select(const sql::SelectStatement& query);
            int execute_insert(const sql::InsertStatement& query);
            int execute_update(const sql::UpdateStatement& query);
            int execute_delete(const sql::DeleteStatement& query);
    };
}
