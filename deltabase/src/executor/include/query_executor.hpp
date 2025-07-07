#ifndef QUERY_EXECUTOR_HPP
#define QUERY_EXECUTOR_HPP

#include "semantic_analyzer.hpp"
#include <string>

extern "C" {
    #include "../../core/include/data_table.h"
    #include "../../core/include/data_filter.h"
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

            int execute_seq_scan(const std::string& db_name, 
                const std::string& table_name, 
                const std::vector<std::string>& column_names, 
                size_t columns_count, 
                const DataFilter& filter, 
                DataTable &out
            );

    };
}

#endif
