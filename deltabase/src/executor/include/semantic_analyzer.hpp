#pragma once

#include "../../storage/include/storage.hpp"
#include "../../engine/include/config.hpp"
#include <optional>
#include <set>

namespace exe {
    inline auto
    get_compatibility_table() -> const std::set<std::pair<sql::SqlLiteral, storage::ValueType>>& {
        static std::set<std::pair<sql::SqlLiteral, storage::ValueType>> map = {
            {sql::SqlLiteral::STRING, storage::ValueType::STRING},
            {sql::SqlLiteral::INTEGER, storage::ValueType::INTEGER},
            {sql::SqlLiteral::INTEGER, storage::ValueType::REAL},
            {sql::SqlLiteral::REAL, storage::ValueType::REAL},
            {sql::SqlLiteral::CHAR, storage::ValueType::CHAR},
            {sql::SqlLiteral::CHAR, storage::ValueType::STRING},
            {sql::SqlLiteral::BOOL, storage::ValueType::BOOL},
        };
        return map;
    }

    inline auto
    is_literal_assignable_to(sql::SqlLiteral literal_type, storage::ValueType column_type) -> bool {
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
        storage::storage& storage_;
        std::optional<std::string> db_name_;
        std::string def_schema_;

        auto
        analyze_select(sql::SelectStatement& stmt) -> AnalysisResult;

        auto
        analyze_insert(sql::InsertStatement& stmt) -> AnalysisResult;

        auto
        analyze_update(sql::UpdateStatement& stmt) -> AnalysisResult;

        auto
        analyze_delete(sql::DeleteStatement& stmt) -> AnalysisResult;

        auto
        analyze_create_table(sql::CreateTableStatement& stmt) -> AnalysisResult;

        auto
        analyze_create_db(sql::CreateDbStatement& stmt) -> AnalysisResult;

        auto
        analyze_create_schema(sql::CreateSchemaStatement& stmt) -> AnalysisResult;

        auto
        analyze_where(std::unique_ptr<sql::AstNode>& where, const storage::MetaTable& table) -> AnalysisResult;

        auto
        validate_column_comparison(
            const std::unique_ptr<sql::AstNode>& left,
            const std::unique_ptr<sql::AstNode>& right,
            const storage::MetaTable& table
        ) -> AnalysisResult;

        auto
        validate_column_assignment(
            const sql::AstNode& assignment, const storage::MetaTable& table
        ) -> AnalysisResult;

        void
        ensure_db_exists();

        void
        ensure_db_exists(const std::string& name);

      public:
        SemanticAnalyzer(storage::storage& storage);
        SemanticAnalyzer(storage::storage& storage, std::string db_name);
        SemanticAnalyzer(storage::storage& storage, engine::EngineConfig cfg);

        auto
        analyze(sql::AstNode& ast) -> AnalysisResult;
    };
}; // namespace exe
