#pragma once

#include "../../storage/include/storage.hpp"
#include "../../engine/include/config.hpp"
#include <optional>
#include <set>

namespace exe
{
    inline auto
    get_compatibility_table() -> const std::set<std::pair<sql::SqlLiteral, storage::ValueType> >&
    {
        static std::set<std::pair<sql::SqlLiteral, storage::ValueType> > map = {
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
    is_literal_assignable_to(sql::SqlLiteral literal_type, storage::ValueType column_type) -> bool
    {
        return get_compatibility_table().count(std::make_pair(literal_type, column_type)) > 0;
    }

    struct AnalysisResult
    {
        bool is_valid;
        std::optional<std::runtime_error> err;

        std::optional<bool> is_system_query;

        AnalysisResult(bool is_valid) : is_valid(is_valid)
        {
        }

        AnalysisResult(std::runtime_error err) : is_valid(false), err(err)
        {
        }

        AnalysisResult(std::runtime_error err, bool is_system_query)
            : is_valid(false), err(err), is_system_query(is_system_query)
        {
        }

        AnalysisResult(bool is_valid,
                       std::optional<std::runtime_error> err,
                       std::optional<bool> is_system_query)
            : is_valid(is_valid), err(std::move(err)), is_system_query(is_system_query)
        {
        }
    };

    class SemanticAnalyzer
    {
        storage::Storage& storage_;
        std::optional<std::string> db_name_;
        std::string def_schema_;

        AnalysisResult
        analyze_select(sql::SelectStatement& stmt) const;

        AnalysisResult
        analyze_insert(sql::InsertStatement& stmt) ;

        AnalysisResult
        analyze_update(sql::UpdateStatement& stmt) ;

        AnalysisResult
        analyze_delete(sql::DeleteStatement& stmt) ;

        AnalysisResult
        analyze_create_table(const sql::CreateTableStatement& stmt) const;

        AnalysisResult
        analyze_create_db(const sql::CreateDbStatement& stmt) const;

        AnalysisResult
        analyze_create_schema(sql::CreateSchemaStatement& stmt) const;

        static AnalysisResult
        analyze_where(const sql::BinaryExpr& where,
                      const storage::MetaTable& table);

        static AnalysisResult
        validate_column_comparison(
            const std::unique_ptr<sql::AstNode>& left,
            const std::unique_ptr<sql::AstNode>& right,
            const storage::MetaTable& table
        );

        static AnalysisResult
        validate_column_assignment(
            const sql::AstNode& assignment,
            const storage::MetaTable& table
        );

        void
        ensure_db_exists() const;

        void
        ensure_db_exists(const std::string& name) const;

    public:
        SemanticAnalyzer(storage::Storage& storage);

        SemanticAnalyzer(storage::Storage& storage, std::string db_name);

        SemanticAnalyzer(storage::Storage& storage, engine::EngineConfig cfg);

        AnalysisResult
        analyze(sql::AstNode& ast) ;
    };
}; // namespace exe