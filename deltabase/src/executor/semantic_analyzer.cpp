//
// Created by poproshaikin on 04.12.25.
//

#include "semantic_analyzer.hpp"

#include "evaluator.hpp"
#include "../misc/include/convert.hpp"
#include "../misc/include/exceptions.hpp"

#include <format>
#include <unordered_set>

namespace exq
{
    using namespace types;

    SemanticAnalyzer::SemanticAnalyzer(const Config& config, storage::IDbInstance& db)
        : db_(db), config_(config), generic_validator_(config)
    {
    }

    AnalysisResult
    SemanticAnalyzer::analyze(const AstNode& node)
    {
        switch (node.type)
        {
        case AstNodeType::SELECT:
            return analyze_select(std::get<SelectStatement>(node.value));

        case AstNodeType::INSERT:
            return analyze_insert(std::get<InsertStatement>(node.value));

        case AstNodeType::UPDATE:
            return analyze_update(std::get<UpdateStatement>(node.value));

        case AstNodeType::DELETE:
            return analyze_delete(std::get<DeleteStatement>(node.value));

        case AstNodeType::CREATE_DATABASE:
            return analyze_create_db(std::get<CreateDbStatement>(node.value));

        case AstNodeType::CREATE_TABLE:
            return analyze_create_table(std::get<CreateTableStatement>(node.value));

        case AstNodeType::CREATE_INDEX:
            return analyze_create_index(std::get<CreateIndexStatement>(node.value));

        case AstNodeType::ALTER_TABLE:
            return analyze_alter_table(std::get<AlterTableStatement>(node.value));

        case AstNodeType::DROP_INDEX:
            return analyze_drop_index(std::get<DropIndexStatement>(node.value));

        case AstNodeType::DROP_TABLE:
            return analyze_drop_table(std::get<DropTableStatement>(node.value));

        default:
            throw std::runtime_error(
                "SemanticAnalyzer::analyze: Unsupported AST node type for semantic analysis"
            );
        }
    }

    AnalysisResult
    SemanticAnalyzer::analyze_select(const SelectStatement& stmt)
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(std::runtime_error("Select statement missing target table"));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto* table = db_.get_table(stmt.table);

        for (const SqlToken& col : stmt.columns)
            if (!table->has_column(col.value))
                return AnalysisResult(ColumnDoesntExist(col.value));

        if (stmt.where.has_value())
        {
            auto where_result = analyze_where(stmt.where.value(), *table);
            if (!where_result.is_valid)
                return AnalysisResult(where_result.err.value());
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_insert(const InsertStatement& stmt) const
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(std::runtime_error("Insert statement missing target table"));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto* table = db_.get_table(stmt.table);

        for (const SqlToken& col : stmt.columns)
            if (!table->has_column(col.value))
                return AnalysisResult(ColumnDoesntExist(col.value));

        const bool has_explicit_columns = !stmt.columns.empty();
        const size_t expected_value_count = has_explicit_columns ? stmt.columns.size() : table->columns.size();

        for (const auto& values : stmt.values)
        {
            if (has_explicit_columns)
            {
                if (values.values.size() != expected_value_count)
                    return AnalysisResult(
                        std::runtime_error("VALUES count does not match columns count"));
            }
            else if (values.values.size() > expected_value_count)
            {
                return AnalysisResult(
                    std::runtime_error("VALUES count exceeds table column count"));
            }
        }

        auto get_column = [&stmt, &table, has_explicit_columns](size_t value_idx)
            -> std::optional<std::reference_wrapper<const MetaColumn>>
        {
            if (has_explicit_columns)
            {
                if (value_idx >= stmt.columns.size())
                    return std::nullopt;

                const auto& name = stmt.columns[value_idx];
                if (!table->has_column(name.value))
                    return std::nullopt;

                return std::cref(table->get_column(name.value));
            }

            if (value_idx >= table->columns.size())
                return std::nullopt;

            return std::cref(table->get_column(static_cast<int64_t>(value_idx)));
        };

        for (const auto& values : stmt.values)
        {
            std::unordered_set<std::string> specified_columns;
            specified_columns.reserve(stmt.columns.size());
            for (const auto& col : stmt.columns)
                specified_columns.insert(col.value);

            for (size_t i = 0; i < values.values.size(); ++i)
            {
                const SqlToken& value = values.values[i];
                if (!std::holds_alternative<SqlLiteral>(value.detail))
                    return AnalysisResult(std::runtime_error("Invalid literal token type"));

                auto column = get_column(i);
                if (!column.has_value())
                    return AnalysisResult("What the fuck is happened");

                auto literal_type = std::get<SqlLiteral>(value.detail);
                auto column_type = column.value().get().type;
                auto is_not_null = has_not_null_constraint(column.value().get());

                if (is_not_null && literal_type == SqlLiteral::_NULL)
                    return AnalysisResult(
                        std::runtime_error(
                            "Cannot insert NULL to non-nullable column"));

                if (!is_compatible(literal_type, column_type))
                    return AnalysisResult(
                        std::runtime_error(
                            std::format(
                                "Incompatible types conversion: {} to {}",
                                static_cast<int>(literal_type),
                                static_cast<int>(column_type)
                            )));
            }

            if (!has_explicit_columns)
            {
                for (size_t i = values.values.size(); i < table->columns.size(); ++i)
                {
                    const auto& column = table->get_column(static_cast<int64_t>(i));
                    if (has_not_null_constraint(column) && !has_default_constraint(column))
                    {
                        return AnalysisResult(std::runtime_error(
                            "INSERT is missing a value for NOT NULL column '" + column.name +
                            "' without DEFAULT"));
                    }
                }
            }
            else
            {
                for (const auto& column : table->columns)
                {
                    if (specified_columns.contains(column.name))
                        continue;

                    if (has_not_null_constraint(column) && !has_default_constraint(column))
                    {
                        return AnalysisResult(std::runtime_error(
                            "INSERT is missing a value for NOT NULL column '" + column.name +
                            "' without DEFAULT"));
                    }
                }
            }
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_update(const UpdateStatement& stmt)
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(std::runtime_error("Update statement missing target table"));

        if (stmt.assignments.empty())
            return AnalysisResult(std::runtime_error("Update statement missing assignments"));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto* table = db_.get_table(stmt.table);

        for (const auto& assignment : stmt.assignments)
        {
            auto assignment_analysis = analyze_column_assignment(assignment, *table);
            if (!assignment_analysis.is_valid)
                return AnalysisResult(*assignment_analysis.err);
        }

        if (stmt.where.has_value())
        {
            auto where_result = analyze_where(*stmt.where, *table);
            if (!where_result.is_valid)
                return AnalysisResult(*where_result.err);
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_delete(const DeleteStatement& stmt)
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(std::runtime_error("Delete statement missing target table"));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto* table = db_.get_table(stmt.table);

        if (stmt.where.has_value())
        {
            auto where_result = analyze_where(*stmt.where, *table);
            if (!where_result.is_valid)
                return AnalysisResult(where_result.err.value());
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_table(const CreateTableStatement& stmt) const
    {
        if (db_.exists_table(stmt.table))
            return AnalysisResult(TableExists(stmt.table.table_name.value));

        return AnalysisResult(true);
    }

    bool
    SemanticAnalyzer::has_not_null_constraint(const MetaColumn& column) const
    {
        for (const auto& constraint : column.constraints)
        {
            if (std::holds_alternative<MetaNotNullConstraint>(constraint))
                return true;
        }
        return false;
    }

    bool
    SemanticAnalyzer::has_default_constraint(const MetaColumn& column) const
    {
        for (const auto& constraint : column.constraints)
        {
            if (std::holds_alternative<MetaDefaultConstraint>(constraint))
                return true;
        }
        return false;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_alter_table(const AlterTableStatement& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto* mt = db_.get_table(stmt.table);

        for (const auto& operation : stmt.operations)
        {
            if (auto* add_col = std::get_if<AddColumnOperation>(&operation))
            {
                if (mt->has_column(add_col->column.name.value))
                    return AnalysisResult(ColumnExists(add_col->column.name.value));

                bool has_not_null = false;
                bool has_default = false;

                for (const auto& constraint : add_col->column.constraints)
                {
                    if (std::holds_alternative<NotNullConstraint>(constraint))
                    {
                        has_not_null = true;
                    }
                    else if (auto* default_value = std::get_if<DefaultConstraint>(&constraint))
                    {
                        has_default = true;

                        auto value_type = default_value->value.get_detail<SqlLiteral>();
                        DataType col_type = misc::convert_to_dt(add_col->column.type);
                        if (!is_compatible(value_type, col_type))
                            return AnalysisResult(
                                std::runtime_error(
                                    "Type of the default value is not compatible with the column's type"));
                    }
                }

                if (has_not_null && mt->live_rows > 0 && !has_default)
                {
                    return AnalysisResult(std::runtime_error(
                        "Cannot add NOT NULL column to non-empty table without DEFAULT value"));
                }

                if (mt->live_rows > 0)
                {
                    for (const auto& existing_col : mt->columns)
                    {
                        if (has_not_null_constraint(existing_col) && !has_default_constraint(existing_col))
                        {
                            return AnalysisResult(std::runtime_error(
                                "Cannot add column to non-empty table: existing column '" + existing_col.name +
                                "' has NOT NULL constraint without DEFAULT value"));
                        }
                    }
                }
            }
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_index(const CreateIndexStatement& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto* table = db_.get_table(stmt.table);

        for (const auto& index : table->indexes)
            if (index.name == stmt.index_name.value)
                return AnalysisResult(std::runtime_error("Index already exists"));

        if (!table->has_column(stmt.column_name.value))
            return AnalysisResult(std::runtime_error("Column doesn't exists"));

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_drop_index(const DropIndexStatement& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        if (!db_.exists_index(stmt.index_name.value, stmt.table))
            return AnalysisResult(
                IndexDoesntExist(stmt.index_name.value, stmt.table.table_name.value));

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_drop_table(const DropTableStatement& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_where(const BinaryExpr& where, const MetaTable& table)
    {
        if (where.op == AstOperator::ASSIGN)
            return AnalysisResult(std::runtime_error("Invalid condition operator"));

        if (where.op == AstOperator::EQ || where.op == AstOperator::NEQ ||
            where.op == AstOperator::GR || where.op == AstOperator::LT ||
            where.op == AstOperator::GRE || where.op == AstOperator::LTE ||
            where.op == AstOperator::IS)
        {

            if (!where.left || !where.right)
                return AnalysisResult(std::runtime_error("Incomplete comparison expression"));

            auto comparison_analysis = analyze_column_comparison(
                where.op,
                where.left,
                where.right,
                table
            );
            if (!comparison_analysis.is_valid)
                return AnalysisResult(*comparison_analysis.err);
        }

        if (where.op == AstOperator::AND || where.op == AstOperator::OR)
        {
            auto where1_analysis = analyze_where(std::get<BinaryExpr>(where.left->value), table);
            auto where2_analysis = analyze_where(std::get<BinaryExpr>(where.right->value), table);

            if (!where1_analysis.is_valid)
                return AnalysisResult(*where1_analysis.err);

            if (!where2_analysis.is_valid)
                return AnalysisResult(*where2_analysis.err);
        }

        if (where.op == AstOperator::NOT)
        {
            auto where_analysis = analyze_where(std::get<BinaryExpr>(where.left->value), table);
            if (!where_analysis.is_valid)
                return AnalysisResult(*where_analysis.err);
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_db(const CreateDbStatement& stmt) const
    {
        if (generic_validator_.exists_db(stmt.name.value))
            return AnalysisResult(static_cast<std::runtime_error>(DbExists(stmt.name.value)), true);

        return AnalysisResult(true, true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_column_assignment(
        const BinaryExpr& expr,
        const MetaTable& table
    ) const
    {
        if (expr.op != AstOperator::ASSIGN)
            return AnalysisResult(std::runtime_error("Invalid assignment: expected '='"));

        if (expr.left->type != AstNodeType::IDENTIFIER || expr.right->type != AstNodeType::LITERAL)
            return AnalysisResult(
                std::runtime_error(
                    "Invalid assignment: you can assign only literal to a identifier"
                )
            );

        expr.left->type = AstNodeType::COLUMN_IDENTIFIER;

        const AstNode* column_node = expr.left.get();
        const AstNode* value_node = expr.right.get();

        const std::string& col_name = std::get<SqlToken>(column_node->value).value;
        if (!table.has_column(col_name))
            return AnalysisResult(ColumnDoesntExist(std::string(col_name)));

        const auto& column = table.get_column(col_name);

        // TODO add support of assigning the value of an other column
        const auto& value_token = std::get<SqlToken>(value_node->value);
        auto literal_type = std::get<SqlLiteral>(value_token.detail);
        if (!is_compatible(literal_type, column.type))
            return AnalysisResult(
                std::runtime_error("Incompatible types conversion in assignment")
            );

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_column_comparison(
        types::AstOperator op,
        const std::unique_ptr<AstNode>& left,
        const std::unique_ptr<AstNode>& right,
        const MetaTable& table
    ) const
    {
        const AstNode* column_node = nullptr;
        const AstNode* value_node = nullptr;

        if (left->type == AstNodeType::IDENTIFIER && right->type == AstNodeType::LITERAL)
        {
            column_node = left.get();
            value_node = right.get();
        }
        else if (right->type == AstNodeType::IDENTIFIER && left->type == AstNodeType::LITERAL)
        {
            column_node = right.get();
            value_node = left.get();
        }
        else
            return AnalysisResult(
                std::runtime_error(
                    "Invalid WHERE expression: comparison must be between column and literal"
                )
            );

        const auto& column_token = std::get<SqlToken>(column_node->value);
        const auto& value_token = std::get<SqlToken>(value_node->value);

        const auto literal_type = std::get<SqlLiteral>(value_token.detail);
        if (op == AstOperator::IS && literal_type != SqlLiteral::_NULL)
            return AnalysisResult(std::runtime_error("IS operator can only be used with NULL"));

        if (!table.has_column(column_token.value))
            return AnalysisResult(ColumnDoesntExist(column_token.value));

        const auto& column = table.get_column(column_token.value);
        if (!is_compatible(literal_type, column.type))
            return AnalysisResult(std::runtime_error("Incompatible types conversion"));

        return AnalysisResult(true);
    }

    bool
    SemanticAnalyzer::is_compatible(SqlLiteral lit, DataType col) const
    {
        if (lit == SqlLiteral::_NULL)
            return true;

        const auto& table = compat_table();

        auto it = table.find(col);
        if (it == table.end())
            return false;

        const auto& arr = it->second;
        if (arr.size() == 0)
            return false;

        auto literal_to_dt_table = literal_to_data_type_table();
        auto convert_to_dt = [&literal_to_dt_table](SqlLiteral literal) -> DataType
        {
            auto it = literal_to_dt_table.find(literal);
            if (it != literal_to_dt_table.end())
                return it->second;
            return DataType::UNDEFINED;
        };

        for (auto type : arr)
            if (type == convert_to_dt(lit))
                return true;

        return false;
    }

} // namespace exq