#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include "../../sql/include/parser.hpp"
#include "../../catalog/include/meta_registry.hpp"
#include <optional>
#include <set>

extern "C" {
#include "../../core/include/meta.h"
}

namespace exe {
    inline const std::set<std::pair<sql::SqlLiteral, DataType>>&
    get_compatibility_table() {
        static std::set<std::pair<sql::SqlLiteral, DataType>> map = {
            {sql::SqlLiteral::STRING, DT_STRING},
            {sql::SqlLiteral::INTEGER, DT_INTEGER},
            {sql::SqlLiteral::INTEGER, DT_REAL},
            {sql::SqlLiteral::REAL, DT_REAL},
            {sql::SqlLiteral::CHAR, DT_CHAR},
            {sql::SqlLiteral::CHAR, DT_STRING},
            {sql::SqlLiteral::BOOL, DT_BOOL},
        };
        return map;
    }

    inline bool
    is_literal_assignable_to(sql::SqlLiteral literal_type, DataType column_type) {
        return get_compatibility_table().count(std::make_pair(literal_type, column_type)) > 0;
    }

    struct AnalysisResult {
        bool is_valid;
        std::optional<std::runtime_error> err;

        std::optional<bool> is_system_query;

        AnalysisResult(bool is_valid) : is_valid(is_valid) {
        }

        AnalysisResult(std::runtime_error err) : is_valid(false), err(err) {
        }

        AnalysisResult(std::runtime_error err, bool is_system_query)
            : is_valid(false), err(err), is_system_query(is_system_query) {
        }

        AnalysisResult(bool is_valid,
                       std::optional<std::runtime_error> err,
                       std::optional<bool> is_system_query)
            : is_valid(is_valid), err(err), is_system_query(is_system_query) {
        }
    };

    class SemanticAnalyzer {
        catalog::MetaRegistry& registry;
        std::optional<std::string> db_name;

        AnalysisResult
        analyze_select(const sql::SelectStatement& stmt);

        AnalysisResult
        analyze_insert(const sql::InsertStatement& stmt);

        AnalysisResult
        analyze_update(const sql::UpdateStatement& stmt);

        AnalysisResult
        analyze_delete(const sql::DeleteStatement& stmt);

        AnalysisResult
        analyze_create_table(const sql::CreateTableStatement& stmt);

        AnalysisResult
        analyze_create_db(const sql::CreateDbStatement& stmt);

        AnalysisResult
        analyze_where(const std::unique_ptr<sql::AstNode>& where, const catalog::CppMetaTable& table);

        AnalysisResult
        validate_column_comparison(const std::unique_ptr<sql::AstNode>& left,
                                   const std::unique_ptr<sql::AstNode>& right,
                                   const catalog::CppMetaTable& table);
        AnalysisResult
        validate_column_assignment(const sql::AstNode& assignment, const catalog::CppMetaTable& table);

        void
        ensure_db_exists();

        void
        ensure_db_exists(const std::string& name);

        bool
        is_table_virtual(const sql::TableIdentifier& table);

      public:
        SemanticAnalyzer(catalog::MetaRegistry& registry) : registry(registry) {
        }

        SemanticAnalyzer(catalog::MetaRegistry& registry, std::string db_name)
            : registry(registry), db_name(db_name) {
                this->ensure_db_exists(db_name);
        }

        AnalysisResult
        analyze(const sql::AstNode& ast);
    };
}; // namespace exe

#endif
