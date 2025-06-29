#pragma once

#include "../../sql/include/parser.hpp"
#include "../../core/include/data_table.h"
#include <set>

namespace exe {
    inline const std::set<std::pair<sql::SqlLiteral, DataType>>& getCompatibilityTable() {
        static std::set<std::pair<sql::SqlLiteral, DataType>> map = {
            {sql::SqlLiteral::STRING,  DT_STRING},
            {sql::SqlLiteral::INTEGER, DT_INTEGER},
            {sql::SqlLiteral::INTEGER, DT_REAL},
            {sql::SqlLiteral::REAL,    DT_REAL},
            {sql::SqlLiteral::CHAR,    DT_CHAR},
            {sql::SqlLiteral::CHAR,    DT_STRING},
            {sql::SqlLiteral::BOOL,    DT_BOOL},
        };
        return map;
    }

    inline bool is_literal_compatible(sql::SqlLiteral literal_type, DataType column_type) {
        return getCompatibilityTable().count(std::make_pair(literal_type, column_type)) > 0;
    }

    class SemanticAnalyzer {
        public:
            SemanticAnalyzer(std::string db_name);
            void analyze(const sql::AstNode* ast);
        private:
            std::string _db_name;

            void analyze_select(const sql::SelectStatement& selectStmt);
            void analyze_insert(const sql::InsertStatement& insertStmt);
            void analyze_update(const sql::UpdateStatement& updateStmt);
            void analyze_delete(const sql::DeleteStatement& deleteStmt);
            void analyze_where(const std::unique_ptr<sql::AstNode>& where, const MetaTable* table);

            void validate_column_comparison(
                const std::unique_ptr<sql::AstNode>& left,
                const std::unique_ptr<sql::AstNode>& right,
                const MetaTable* table
            );
            void validate_column_assignment(sql::AstNode* assignment, MetaTable *table);
            MetaColumn* ensure_column_exists(const MetaTable* table, std::string colname);
    };
};
