#include "include/semantic_analyzer.hpp"
#include "../misc/include/exceptions.hpp"
#include <format>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <variant>

namespace exe
{

    SemanticAnalyzer::SemanticAnalyzer(storage::Storage& storage) : storage_(storage)
    {
    }

    SemanticAnalyzer::SemanticAnalyzer(storage::Storage& storage, std::string db_name)
        : storage_(storage), db_name_(db_name)
    {
        this->ensure_db_exists(db_name);
    }

    SemanticAnalyzer::SemanticAnalyzer(storage::Storage& storage, engine::EngineConfig cfg)
        : storage_(storage), db_name_(cfg.db_name), def_schema_(cfg.default_schema)
    {
    }

    sql::TableIdentifier
    normalize_table_identifier(const sql::TableIdentifier& table)
    {
        sql::TableIdentifier normalized = table;
        if (!normalized.schema_name)
        {
            sql::SqlToken public_token;
            public_token.value = "public";
            public_token.type = sql::SqlTokenType::IDENTIFIER;
            normalized.schema_name = public_token;
        }
        return normalized;
    }

    AnalysisResult
    SemanticAnalyzer::analyze(sql::AstNode& ast)
    {
        switch (ast.type)
        {
        case sql::AstNodeType::SELECT:
            return analyze_select(std::get<sql::SelectStatement>(ast.value));

        case sql::AstNodeType::INSERT:
            return analyze_insert(std::get<sql::InsertStatement>(ast.value));

        case sql::AstNodeType::UPDATE:
            return analyze_update(std::get<sql::UpdateStatement>(ast.value));

        case sql::AstNodeType::DELETE:
            return analyze_delete(std::get<sql::DeleteStatement>(ast.value));

        case sql::AstNodeType::CREATE_TABLE:
            return analyze_create_table(std::get<sql::CreateTableStatement>(ast.value));

        case sql::AstNodeType::CREATE_DATABASE:
            return analyze_create_db(std::get<sql::CreateDbStatement>(ast.value));

        case sql::AstNodeType::CREATE_SCHEMA:
            return analyze_create_schema(std::get<sql::CreateSchemaStatement>(ast.value));

        default:
            return std::runtime_error("Unsupported AST node type for semantic analysis");
        }
    }

    AnalysisResult
    SemanticAnalyzer::analyze_select(sql::SelectStatement& stmt) const
    {
        stmt.table = normalize_table_identifier(stmt.table);

        if (stmt.table.table_name.value.empty())
            return std::runtime_error("Select statement missing target table");

        if (!storage_.exists_table(stmt.table) &&
            !storage_.exists_virtual_table(stmt.table))
            return std::runtime_error(TableDoesntExist(stmt.table.table_name.value));

        auto& table = storage_.exists_virtual_table(stmt.table)
                          ? storage_.get_virtual_meta_table(stmt.table)
                          : storage_.get_table(stmt.table);

        for (const sql::SqlToken& col : stmt.columns)
            if (!table.has_column(col.value))
                return std::runtime_error(ColumnDoesntExists(col.value));

        if (auto where_result = analyze_where(stmt.where, table); !where_result.is_valid)
            return where_result.err.value();

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_insert(const sql::InsertStatement& stmt) const
    {
        auto normalized_table = normalize_table_identifier(stmt.table);

        if (stmt.table.table_name.value.empty())
            return std::runtime_error("Insert statement missing target table");

        if (stmt.columns.size() != 0 &&
            stmt.columns.size() != stmt.values.size())
            return std::runtime_error("Insert statement columns count does not match values count");

        if (!storage_.exists_table(normalized_table))
            return AnalysisResult(TableDoesntExist(normalized_table.table_name.value));

        const auto& table = storage_.get_table(normalized_table.table_name);

        for (size_t i = 0; i < stmt.columns.size(); i++)
        {
            if (!table.has_column(stmt.columns[i].value))
                return std::runtime_error("Column doesn't exist");

            const sql::SqlToken& value_token = stmt.values[i];
            const sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
            const auto& column = table.get_column(stmt.columns[i].value);

            if (!is_literal_assignable_to(literal_type, column.type))
                return std::runtime_error("Incompatible types conversion");
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_update(sql::UpdateStatement& stmt) const
    {
        if (stmt.table.table_name.value.empty())
            return std::runtime_error("Update statement missing target table");

        if (stmt.assignments.empty())
            return std::runtime_error("Update statement missing assignments");

        if (!storage_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto& table = storage_.get_table(stmt.table.table_name);

        for (const auto& assignment : stmt.assignments)
            validate_column_assignment(assignment, table);

        auto where_result = analyze_where(stmt.where, table);
        if (!where_result.is_valid)
            return where_result.err.value();

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_delete(sql::DeleteStatement& stmt) const
    {
        if (stmt.table.table_name.value.empty())
            return std::runtime_error("Delete statement missing target table");

        if (!storage_.exists_table(stmt.table))
            return AnalysisResult(TableDoesntExist(stmt.table.table_name.value));

        const auto& table = storage_.get_table(stmt.table.table_name);

        auto where_result = analyze_where(stmt.where, table);
        if (!where_result.is_valid)
            return where_result.err.value();

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_table(const sql::CreateTableStatement& stmt) const
    {
        if (storage_.exists_table(stmt.table))
        {
            return AnalysisResult(TableExists(stmt.table.table_name.value));
        }
        return true;
    }

    AnalysisResult
    SemanticAnalyzer::validate_column_comparison(const std::unique_ptr<sql::AstNode>& left,
                                                 const std::unique_ptr<sql::AstNode>& right,
                                                 const storage::MetaTable& table)
    {
        const sql::AstNode* column_node = nullptr;
        const sql::AstNode* value_node = nullptr;

        if (left->type == sql::AstNodeType::IDENTIFIER &&
            right->type == sql::AstNodeType::LITERAL)
        {
            column_node = left.get();
            value_node = right.get();
        }
        else if (right->type == sql::AstNodeType::IDENTIFIER &&
                 left->type == sql::AstNodeType::LITERAL)
        {
            column_node = right.get();
            value_node = left.get();
        }
        else
            return std::runtime_error(
                "Invalid WHERE expression: comparison must be between column and literal");

        const auto& column_token = std::get<sql::SqlToken>(column_node->value);
        const auto& value_token = std::get<sql::SqlToken>(value_node->value);

        const auto literal_type = std::get<sql::SqlLiteral>(value_token.detail);
        if (!table.has_column(column_token.value))
            return AnalysisResult(ColumnDoesntExists(column_token.value));

        const auto& column = table.get_column(column_token.value);
        if (!is_literal_assignable_to(literal_type, column.type))
            return std::runtime_error("Incompatible types conversion");

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::validate_column_assignment(const sql::AstNode& assignment,
                                                 const storage::MetaTable& table)
    {
        if (!std::holds_alternative<sql::BinaryExpr>(assignment.value))
            return std::runtime_error("Invalid assignment");

        const auto& expr = std::get<sql::BinaryExpr>(assignment.value);
        if (expr.op != sql::AstOperator::ASSIGN)
        {
            return std::runtime_error("Invalid assignment: expected '='");
        }

        if (expr.left->type != sql::AstNodeType::IDENTIFIER ||
            expr.right->type != sql::AstNodeType::LITERAL)
            return std::runtime_error(
                "Invalid assignment: you can assign only literal to a identifier");

        const sql::AstNode* column_node = expr.left.get();
        const sql::AstNode* value_node = expr.right.get();

        const std::string& col_name = std::get<sql::SqlToken>(column_node->value).value;
        if (!table.has_column(col_name))
            return AnalysisResult(ColumnDoesntExists(std::string(col_name)));

        const auto& column = table.get_column(col_name);

        const auto& value_token = std::get<sql::SqlToken>(value_node->value);
        auto literal_type = std::get<sql::SqlLiteral>(value_token.detail);
        if (!is_literal_assignable_to(literal_type, column.type))
            return std::runtime_error("Incompatible types conversion in assignment");

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_where(const sql::BinaryExpr& where,
                                    const storage::MetaTable& table)
    {
        using sql::AstNodeType;
        using sql::AstOperator;

        if (where.op == AstOperator::ASSIGN)
            return {std::runtime_error("Invalid condition operator")};

        if (where.op == AstOperator::EQ || where.op == AstOperator::NEQ ||
            where.op == AstOperator::GR || where.op == AstOperator::LT ||
            where.op == AstOperator::GRE || where.op == AstOperator::LTE)
        {

            if (!where.left || !where.right)
                return std::runtime_error("Incomplete comparison expression");

            validate_column_comparison(where.left, where.right, table);
        }

        if (where.op == AstOperator::AND || where.op == AstOperator::OR)
        {
            analyze_where(std::get<sql::BinaryExpr>(where.left->value), table);
            analyze_where(std::get<sql::BinaryExpr>(where.right->value), table);
        }

        if (where.op == AstOperator::NOT)
            analyze_where(std::get<sql::BinaryExpr>(where.left->value), table);

        return true;
    }

    void
    SemanticAnalyzer::ensure_db_exists(const std::string& name) const
    {
        if (!storage_.exists_database(name.c_str()))
        {
            throw DbDoesntExists(name);
        }
    }

    void
    SemanticAnalyzer::ensure_db_exists() const
    {
        ensure_db_exists(*db_name_);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_db(const sql::CreateDbStatement& stmt) const
    {
        if (storage_.exists_database(stmt.name.value.c_str()))
        {
            return AnalysisResult(DbExists(stmt.name.value));
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_schema(
        sql::CreateSchemaStatement& stmt) const
    {
        if (storage_.exists_schema(stmt.name.value))
            return std::runtime_error(std::format("Schema {} already exists", stmt.name.value));

        return true;
    }
} // namespace exe