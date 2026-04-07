//
// Created by poproshaikin on 03.12.25.
//

#ifndef DELTABASE_SEMANTIC_ANALYZER_HPP
#define DELTABASE_SEMANTIC_ANALYZER_HPP
#include "generic_query_validator.hpp"
#include "../../types/include/analysis_result.hpp"
#include "../../storage/include/db_instance.hpp"

namespace exq
{
    class SemanticAnalyzer
    {
        storage::IDbInstance& db_;
        GenericQueryValidator generic_validator_;
        types::Config config_;

        types::AnalysisResult
        analyze_select(const types::SelectStatement& stmt);

        types::AnalysisResult
        analyze_insert(const types::InsertStatement& stmt) const;

        types::AnalysisResult
        analyze_update(const types::UpdateStatement& stmt);

        types::AnalysisResult
        analyze_delete(const types::DeleteStatement& stmt);

        types::AnalysisResult
        analyze_create_table(const types::CreateTableStatement& stmt) const;

        types::AnalysisResult
        analyze_create_index(const types::CreateIndexStatement& stmt) const;

        types::AnalysisResult
        analyze_drop_index(const types::DropIndexStatement& stmt) const;

        types::AnalysisResult
        analyze_create_db(const types::CreateDbStatement& stmt) const;

        types::AnalysisResult
        analyze_where(const types::BinaryExpr& where, const types::MetaTable& table);

        types::AnalysisResult
        analyze_column_assignment(
            const types::BinaryExpr &expr,
            const types::MetaTable& table
        ) const;

        types::AnalysisResult
        analyze_column_comparison(
            types::AstOperator op,
            const std::unique_ptr<types::AstNode>& left,
            const std::unique_ptr<types::AstNode>& right,
            const types::MetaTable& table
        ) const;

        static const std::unordered_map<types::DataType, std::vector<types::DataType> >&
        compat_table()
        {
            static const std::unordered_map<types::DataType, std::vector<types::DataType> >
                table =
                {
                    { types::DataType::INTEGER, { types::DataType::INTEGER } },
                    { types::DataType::REAL, { types::DataType::REAL, types::DataType::INTEGER } },
                    { types::DataType::STRING, { types::DataType::STRING, types::DataType::CHAR } },
                    { types::DataType::CHAR, { types::DataType::CHAR } },
                    { types::DataType::BOOL, { types::DataType::BOOL } },
                    { types::DataType::_NULL, { types::DataType::_NULL, types::DataType::INTEGER, types::DataType::REAL, types::DataType::STRING, types::DataType::CHAR, types::DataType::BOOL } }
                };

            return table;
        }

        static const std::unordered_map<types::SqlLiteral, types::DataType>&
        literal_to_data_type_table()
        {
            static const std::unordered_map<types::SqlLiteral, types::DataType> data_type_table =
            {
                { types::SqlLiteral::INTEGER, types::DataType::INTEGER },
                { types::SqlLiteral::STRING, types::DataType::STRING },
                { types::SqlLiteral::CHAR, types::DataType::CHAR },
                { types::SqlLiteral::BOOL, types::DataType::BOOL },
                { types::SqlLiteral::REAL, types::DataType::REAL },
                { types::SqlLiteral::NULL_, types::DataType::_NULL },
            };
            return data_type_table;
        }


        bool
        is_compatible(types::SqlLiteral lit, types::DataType col) const;

    public:
        explicit
        SemanticAnalyzer(const types::Config& config, storage::IDbInstance& db);

        types::AnalysisResult
        analyze(const types::AstNode& node);
    };
}

#endif //DELTABASE_SEMANTIC_ANALYZER_HPP