#pragma once

#include "../../storage/include/storage.hpp"
#include "../../catalog/include/meta_registry.hpp"
#include "../../engine/include/config.hpp"
#include <optional>
#include <set>
#include <utility>

namespace exe {
    inline auto
    get_compatibility_table() -> const std::set<std::pair<sql::SqlLiteral, DataType>>& {
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

    inline auto
    is_literal_assignable_to(sql::SqlLiteral literal_type, DataType column_type) -> bool {
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
            : is_valid(is_valid), err(std::move(err)), is_system_query(is_system_query) {
        }
    };

    class SemanticAnalyzer {
        catalog::MetaRegistry& registry_;
        std::optional<std::string> db_name_;
        std::string def_schema_;

        auto
        analyze_select(const sql::SelectStatement& stmt) -> AnalysisResult;

        auto
        analyze_insert(const sql::InsertStatement& stmt) -> AnalysisResult;

        auto
        analyze_update(const sql::UpdateStatement& stmt) -> AnalysisResult;

        auto
        analyze_delete(const sql::DeleteStatement& stmt) -> AnalysisResult;

        auto
        analyze_create_table(const sql::CreateTableStatement& stmt) -> AnalysisResult;

        auto
        analyze_create_db(const sql::CreateDbStatement& stmt) -> AnalysisResult;

        auto
        analyze_create_schema(const sql::CreateSchemaStatement& stmt) -> AnalysisResult;

        auto
        analyze_where(const std::unique_ptr<sql::AstNode>& where, const catalog::CppMetaTable& table) -> AnalysisResult;

        auto
        validate_column_comparison(
            const std::unique_ptr<sql::AstNode>& left,
            const std::unique_ptr<sql::AstNode>& right,
            const catalog::CppMetaTable& table
        ) -> AnalysisResult;

        auto
        validate_column_assignment(
            const sql::AstNode& assignment, const catalog::CppMetaTable& table
        ) -> AnalysisResult;

        void
        ensure_db_exists();

        void
        ensure_db_exists(const std::string& name);

      public:
        SemanticAnalyzer(catalog::MetaRegistry& registry);
        SemanticAnalyzer(catalog::MetaRegistry& registry, std::string db_name);
        SemanticAnalyzer(catalog::MetaRegistry& registry, engine::EngineConfig cfg);

        auto
        analyze(const sql::AstNode& ast) -> AnalysisResult;
    };
}; // namespace exe
