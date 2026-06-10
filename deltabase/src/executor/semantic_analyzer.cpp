//
// Created by poproshaikin on 04.12.25.
//

#include "semantic_analyzer.hpp"

#include "evaluator.hpp"
#include "../misc/include/convert.hpp"
#include "../misc/include/exceptions.hpp"
#include "../sql/include/dictionary.hpp"

#include <format>
#include <unordered_set>

namespace exq
{
    using namespace types;

    SemanticAnalyzer::SemanticAnalyzer(const Config& config, storage::IDbInstance& db)
        : db_(db), config_(config), generic_validator_(config), info_schema_provider_(db)
    {
    }

    AnalysisResult
    SemanticAnalyzer::analyze(const AstNode& node)
    {
        switch (node.type)
        {
        case AstNodeType::SELECT:
            return analyze_select(std::get<SelectStmt>(node.value));

        case AstNodeType::INSERT:
            return analyze_insert(std::get<InsertStmt>(node.value));

        case AstNodeType::UPDATE:
            return analyze_update(std::get<UpdateStmt>(node.value));

        case AstNodeType::DELETE:
            return analyze_delete(std::get<DeleteStmt>(node.value));

        case AstNodeType::CREATE_DATABASE:
            return analyze_create_db(std::get<CreateDatabaseStmt>(node.value));

        case AstNodeType::CREATE_TABLE:
            return analyze_create_table(std::get<CreateTableStmt>(node.value));

        case AstNodeType::CREATE_INDEX:
            return analyze_create_index(std::get<CreateIndexStmt>(node.value));

        case AstNodeType::ALTER_TABLE:
            return analyze_alter_table(std::get<AlterTableStmt>(node.value));

        case AstNodeType::DROP_INDEX:
            return analyze_drop_index(std::get<DropIndexStmt>(node.value));

        case AstNodeType::DROP_TABLE:
            return analyze_drop_table(std::get<DropTableStmt>(node.value));

        default:
            throw std::runtime_error(
                "SemanticAnalyzer::analyze: Unsupported AST node type for semantic analysis"
            );
        }
    }

    AnalysisResult
    SemanticAnalyzer::analyze_select(const SelectStmt& stmt)
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(EngineException("Select statement missing target table",
                                                  EngineException::Code::SYNTAX_ERROR));

        const MetaTable* table = nullptr;
        std::optional<MetaTable> virtual_table_storage;

        if (info_schema_provider_.is_virtual(stmt.table))
        {
            virtual_table_storage = info_schema_provider_.get_virtual_table(stmt.table);
            table = &virtual_table_storage.value();
        }
        else
        {
            if (!db_.exists_table(stmt.table))
                return AnalysisResult(EngineException(
                    "Table '" + stmt.table.table_name.value + "' doesn't exist",
                    EngineException::Code::TABLE_NOT_EXISTS));
            table = db_.get_table(stmt.table);
        }

        for (const SqlToken& col : stmt.columns)
            if (!table->has_column(col.value))
                return AnalysisResult(EngineException("Column '" + col.value + "' doesn't exist",
                                                      EngineException::Code::COLUMN_NOT_EXISTS));

        if (stmt.where.has_value())
        {
            auto where_result = analyze_where(stmt.where.value(), *table);
            if (!where_result.is_valid)
                return AnalysisResult(where_result.err.value());
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_insert(const InsertStmt& stmt) const
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(EngineException("Insert statement missing target table",
                                                  EngineException::Code::SYNTAX_ERROR));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

        const auto* table = db_.get_table(stmt.table);

        for (const SqlToken& col : stmt.columns)
            if (!table->has_column(col.value))
                return AnalysisResult(EngineException("Column '" + col.value + "' doesn't exist",
                                                      EngineException::Code::COLUMN_NOT_EXISTS));

        const bool has_explicit_columns = !stmt.columns.empty();
        const size_t expected_value_count = has_explicit_columns
                                                ? stmt.columns.size()
                                                : table->columns.size();

        for (const auto& values : stmt.values)
        {
            if (has_explicit_columns)
            {
                if (values.values.size() != expected_value_count)
                    return AnalysisResult(EngineException(
                        "VALUES count does not match columns count",
                        EngineException::Code::COLUMN_COUNT_MISMATCH));
            }
            else if (values.values.size() > expected_value_count)
            {
                return AnalysisResult(EngineException("VALUES count exceeds table column count",
                                                      EngineException::Code::COLUMN_COUNT_MISMATCH));
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
                    return AnalysisResult(EngineException("Invalid literal token type",
                                                          EngineException::Code::TYPE_MISMATCH));

                auto column = get_column(i);
                if (!column.has_value())
                    return AnalysisResult(EngineException("Internal error: unexpected column state",
                                                          EngineException::Code::GENERIC));

                auto literal_type = std::get<SqlLiteral>(value.detail);
                auto column_type = column.value().get().type;
                auto is_not_null = has_not_null_constraint(column.value().get());

                if (is_not_null && literal_type == SqlLiteral::_NULL)
                    return AnalysisResult(EngineException(
                        "Cannot insert NULL to non-nullable column",
                        EngineException::Code::NOT_NULL_VIOLATION));

                if (!is_compatible(literal_type, column_type))
                    return AnalysisResult(EngineException(
                        std::format("Incompatible types conversion: {} to {}",
                                    static_cast<int>(literal_type),
                                    static_cast<int>(column_type)),
                        EngineException::Code::TYPE_MISMATCH));
            }

            if (!has_explicit_columns)
            {
                for (size_t i = values.values.size(); i < table->columns.size(); ++i)
                {
                    const auto& column = table->get_column(static_cast<int64_t>(i));

                    if (has_not_null_constraint(column) &&
                        !has_default_constraint(column) &&
                        !has_autoincrement_constraint(column))
                        return AnalysisResult(EngineException(
                            "INSERT is missing a value for NOT NULL column '" + column.name +
                            "' without DEFAULT",
                            EngineException::Code::NOT_NULL_VIOLATION));
                }
            }
            else
            {
                for (const auto& column : table->columns)
                {
                    if (specified_columns.contains(column.name))
                        continue;

                    if (has_not_null_constraint(column) &&
                        !has_default_constraint(column) &&
                        !has_autoincrement_constraint(column))
                        return AnalysisResult(EngineException(
                            "INSERT is missing a value for NOT NULL column '" + column.name +
                            "' without DEFAULT",
                            EngineException::Code::NOT_NULL_VIOLATION));
                }
            }
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_update(const UpdateStmt& stmt)
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(EngineException("Update statement missing target table",
                                                  EngineException::Code::SYNTAX_ERROR));

        if (stmt.assignments.empty())
            return AnalysisResult(EngineException("Update statement missing assignments",
                                                  EngineException::Code::SYNTAX_ERROR));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

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
    SemanticAnalyzer::analyze_delete(const DeleteStmt& stmt)
    {
        if (stmt.table.table_name.value.empty())
            return AnalysisResult(EngineException("Delete statement missing target table",
                                                  EngineException::Code::SYNTAX_ERROR));

        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

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
    SemanticAnalyzer::analyze_create_table(const CreateTableStmt& stmt) const
    {
        if (db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' already exists",
                EngineException::Code::TABLE_EXISTS));

        const ColumnDefinition* pk_col = nullptr;
        for (const auto& col_def : stmt.columns)
            for (const auto& c : col_def.constraints)
                if (std::holds_alternative<PrimaryKeyConstraint>(c))
                {
                    if (pk_col == nullptr)
                        pk_col = &col_def;
                    else
                        return AnalysisResult(EngineException(
                            "Table can have at most one primary key",
                            EngineException::Code::MULTIPLE_PK));
                }
                else if (std::holds_alternative<AutoIncrementConstraint>(c))
                {
                    auto data_type = sql::to_data_type(col_def.type.get_detail<SqlKeyword>());
                    if (data_type != DataType::INTEGER && data_type != DataType::REAL)
                        return AnalysisResult(EngineException(
                            "Auto incremented column can only be of a numerical type",
                            EngineException::Code::INVALID_AUTOINCREMENT_TYPE));
                }
                else if (auto* fk = std::get_if<ForeignKeyConstraint>(&c))
                {
                    auto referenced_table = db_.get_table(fk->referenced_table);
                    if (!referenced_table)
                        return AnalysisResult(EngineException(
                            "Referenced table " + fk->referenced_table.table_name.value +
                            " not found",
                            EngineException::Code::REF_TABLE_NOT_EXISTS));

                    if (!referenced_table->has_column(fk->referenced_column.value))
                        return AnalysisResult(EngineException(
                            "Referenced column " + fk->referenced_column.value + " on table " +
                            referenced_table->name + " does not exist",
                            EngineException::Code::REF_COLUMN_NOT_EXISTS));

                    auto referenced_column = referenced_table->get_column(
                        fk->referenced_column.value);
                    DataType referencing_dt = misc::convert_to_dt(col_def.type);

                    if (referencing_dt != referenced_column.type)
                        return AnalysisResult(EngineException(
                            "Referenced column should have the same type as the referencing column",
                            EngineException::Code::REF_COLUMN_TYPE_MISMATCH));

                    if (!referenced_table->is_unique(referenced_column.name))
                        return AnalysisResult(EngineException(
                            "Referenced column '" + referenced_column.name + " must be unique",
                            EngineException::Code::REF_COLUMN_NOT_UNIQUE));

                    bool referencing_column_is_not_null = false;
                    for (const auto& referencing_c : col_def.constraints)
                        if (std::holds_alternative<NotNullConstraint>(referencing_c))
                            referencing_column_is_not_null = true;

                    if (fk->action == OnDeleteFkAction::SET_NULL &&
                        referencing_column_is_not_null)
                        return AnalysisResult(EngineException(
                            "Referencing column has NOT NULL constraint",
                            EngineException::Code::REF_COLUMN_NOT_NULL));
                }

        if (pk_col)
            for (const auto& c : pk_col->constraints)
                if (const auto* dc = std::get_if<DefaultConstraint>(&c))
                {
                    if (dc->value.get_detail<SqlLiteral>() == SqlLiteral::_NULL)
                        return AnalysisResult(EngineException(
                            "Primary key column cannot have DEFAULT NULL",
                            EngineException::Code::NULLABLE_PK));
                    break;
                }

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

    bool
    SemanticAnalyzer::has_autoincrement_constraint(const MetaColumn& column) const
    {
        for (const auto& constraint : column.constraints)
        {
            if (std::holds_alternative<MetaAutoIncrementConstraint>(constraint))
                return true;
        }
        return false;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_alter_table(const AlterTableStmt& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

        const auto* mt = db_.get_table(stmt.table);

        for (const auto& operation : stmt.operations)
        {
            if (auto* add_col = std::get_if<AddColumnOperation>(&operation))
            {
                if (mt->has_column(add_col->column.name.value))
                    return AnalysisResult(EngineException(
                        "Column '" + add_col->column.name.value + "' already exists",
                        EngineException::Code::COLUMN_EXISTS));

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
                            return AnalysisResult(EngineException(
                                "Type of the default value is not compatible with the column's type",
                                EngineException::Code::TYPE_MISMATCH));
                    }
                }

                if (has_not_null && mt->live_rows > 0 && !has_default)
                    return AnalysisResult(EngineException(
                        "Cannot add NOT NULL column to non-empty table without DEFAULT value",
                        EngineException::Code::NOT_NULL_VIOLATION));

                if (mt->live_rows > 0)
                {
                    for (const auto& existing_col : mt->columns)
                    {
                        if (has_not_null_constraint(existing_col) && !has_default_constraint(
                                existing_col))
                            return AnalysisResult(EngineException(
                                "Cannot add column to non-empty table: existing column '" +
                                existing_col.name +
                                "' has NOT NULL constraint without DEFAULT value",
                                EngineException::Code::NOT_NULL_VIOLATION));
                    }
                }
            }
        }

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_index(const CreateIndexStmt& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

        const auto* table = db_.get_table(stmt.table);

        for (const auto& index : table->indexes)
            if (index.name == stmt.index_name.value)
                return AnalysisResult(EngineException(
                    "Index '" + stmt.index_name.value + "' already exists",
                    EngineException::Code::INDEX_EXISTS));

        if (!table->has_column(stmt.column_name.value))
            return AnalysisResult(EngineException(
                "Column '" + stmt.column_name.value + "' doesn't exist",
                EngineException::Code::COLUMN_NOT_EXISTS));

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_drop_index(const DropIndexStmt& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

        if (!db_.exists_index(stmt.index_name.value, stmt.table))
            return AnalysisResult(EngineException(
                "Index '" + stmt.index_name.value + "' does not exist on table " + stmt.table.
                table_name.value,
                EngineException::Code::INDEX_NOT_EXISTS));

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_drop_table(const DropTableStmt& stmt) const
    {
        if (!db_.exists_table(stmt.table))
            return AnalysisResult(EngineException(
                "Table '" + stmt.table.table_name.value + "' doesn't exist",
                EngineException::Code::TABLE_NOT_EXISTS));

        return AnalysisResult(true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_where(const BinaryExpr& where, const MetaTable& table)
    {
        if (where.op == AstOperator::ASSIGN)
            return AnalysisResult(EngineException(
                "Invalid condition operator: ASSIGN cannot be used in WHERE clause",
                EngineException::Code::INVALID_COMPARISON));

        if (where.op == AstOperator::EQ || where.op == AstOperator::NEQ ||
            where.op == AstOperator::GR || where.op == AstOperator::LT ||
            where.op == AstOperator::GRE || where.op == AstOperator::LTE ||
            where.op == AstOperator::IS)
        {

            if (!where.left || !where.right)
                return AnalysisResult(EngineException("Incomplete comparison expression",
                                                      EngineException::Code::SYNTAX_ERROR));

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
    SemanticAnalyzer::analyze_create_db(const CreateDatabaseStmt& stmt) const
    {
        if (generic_validator_.exists_db(stmt.name.value))
            return AnalysisResult(EngineException(
                                      "Database '" + stmt.name.value + "' already exists",
                                      EngineException::Code::DB_EXISTS),
                                  true);

        return AnalysisResult(true, true);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_column_assignment(
        const BinaryExpr& expr,
        const MetaTable& table
    ) const
    {
        if (expr.op != AstOperator::ASSIGN)
            return AnalysisResult(EngineException("Invalid assignment: expected '='",
                                                  EngineException::Code::SYNTAX_ERROR));

        if (expr.left->type != AstNodeType::IDENTIFIER || expr.right->type != AstNodeType::LITERAL)
            return AnalysisResult(EngineException(
                "Invalid assignment: you can assign only literal to a identifier",
                EngineException::Code::SYNTAX_ERROR));

        expr.left->type = AstNodeType::COLUMN_IDENTIFIER;

        const AstNode* column_node = expr.left.get();
        const AstNode* value_node = expr.right.get();

        const std::string& col_name = std::get<SqlToken>(column_node->value).value;
        if (!table.has_column(col_name))
            return AnalysisResult(EngineException(
                "Column '" + std::string(col_name) + "' doesn't exist",
                EngineException::Code::COLUMN_NOT_EXISTS));

        const auto& column = table.get_column(col_name);

        // TODO add support of assigning the value of an other column
        const auto& value_token = std::get<SqlToken>(value_node->value);
        auto literal_type = std::get<SqlLiteral>(value_token.detail);
        if (!is_compatible(literal_type, column.type))
            return AnalysisResult(EngineException("Incompatible types conversion in assignment",
                                                  EngineException::Code::TYPE_MISMATCH));

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
            return AnalysisResult(EngineException(
                "Invalid WHERE expression: comparison must be between column and literal",
                EngineException::Code::SYNTAX_ERROR));

        const auto& column_token = std::get<SqlToken>(column_node->value);
        const auto& value_token = std::get<SqlToken>(value_node->value);

        const auto literal_type = std::get<SqlLiteral>(value_token.detail);
        if (op == AstOperator::IS && literal_type != SqlLiteral::_NULL)
            return AnalysisResult(EngineException("IS operator can only be used with NULL",
                                                  EngineException::Code::INVALID_COMPARISON));

        if (!table.has_column(column_token.value))
            return AnalysisResult(EngineException(
                "Column '" + column_token.value + "' doesn't exist",
                EngineException::Code::COLUMN_NOT_EXISTS));

        const auto& column = table.get_column(column_token.value);
        if (!is_compatible(literal_type, column.type))
            return AnalysisResult(EngineException("Incompatible types conversion",
                                                  EngineException::Code::TYPE_MISMATCH));

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