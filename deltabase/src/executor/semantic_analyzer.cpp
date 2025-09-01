#include "include/semantic_analyzer.hpp"
#include "../misc/include/exceptions.hpp"
#include <stdexcept>
#include <string.h>
#include <variant>

extern "C" {
#include "../core/include/core.h"
}

namespace exe {

    AnalysisResult
    SemanticAnalyzer::analyze(const sql::AstNode& ast) {
        switch (ast.type) {
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

        default:
            return std::runtime_error("Unsupported AST node type for semantic analysis");
        }
    }

    AnalysisResult
    SemanticAnalyzer::analyze_select(const sql::SelectStatement& selectStmt) {
        if (selectStmt.table.table_name.value.empty()) {
            return std::runtime_error("Select statement missing target table");
        }

        if (!this->registry.has_table(selectStmt.table) &&
            !this->registry.has_virtual_table(selectStmt.table)) {
            return TableDoesntExist(selectStmt.table.table_name.value);
        }

        std::unique_ptr<catalog::CppMetaTable> table;
        if (catalog::is_table_virtual(selectStmt.table)) {
            table = std::make_unique<catalog::CppMetaTable>(this->registry.get_virtual_table(selectStmt.table));
        } else {
            table = std::make_unique<catalog::CppMetaTable>(this->registry.get_table(selectStmt.table.table_name.value));
        }

        if (!table) {
            throw std::runtime_error("Registry inconsistency: table exists check passed but retrieval failed - possible race condition or registry corruption");
        }

        for (const sql::SqlToken& col : selectStmt.columns) {
            if (!table->has_column(col.value)) {
                return ColumnDoesntExists(col.value);
            }
        }

        auto where_result = analyze_where(selectStmt.where, *table);
        if (!where_result.is_valid) {
            return where_result.err.value();
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_insert(const sql::InsertStatement& stmt) {
        if (stmt.table.table_name.value.empty()) {
            return std::runtime_error("Insert statement missing target table");
        }
        if (stmt.columns.size() != 0 &&
            stmt.columns.size() != stmt.values.size()) {
            return std::runtime_error("Insert statement columns count does not match values count");
        }

        if (!this->registry.has_table(stmt.table)) {
            return TableDoesntExist(stmt.table.table_name.value);
        }

        catalog::CppMetaTable table = this->registry.get_table(stmt.table.table_name);

        for (size_t i = 0; i < stmt.columns.size(); i++) {
            if (!table.has_column(stmt.columns[i].value)) {
                return std::runtime_error("Column doesn't exist");
            }

            const sql::SqlToken& value_token = stmt.values[i];
            const sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
            const catalog::CppMetaColumn& column = table.get_column(stmt.columns[i].value);            

            if (!is_literal_assignable_to(literal_type, column.get_data_type())) {
                return std::runtime_error("Incompatible types conversion");
            }
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_update(const sql::UpdateStatement& stmt) {
        if (stmt.table.table_name.value.empty()) {
            return std::runtime_error("Update statement missing target table");
        }
        if (stmt.assignments.empty()) {
            return std::runtime_error("Update statement missing assignments");
        }

        if (!this->registry.has_table(stmt.table)) {
            return TableDoesntExist(stmt.table.table_name.value);
        }

        catalog::CppMetaTable table = this->registry.get_table(stmt.table.table_name);

        for (size_t i = 0; i < stmt.assignments.size(); i++) {
            validate_column_assignment(stmt.assignments[i], table);
        }

        auto where_result = analyze_where(stmt.where, table);
        if (!where_result.is_valid) {
            return where_result.err.value();
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_delete(const sql::DeleteStatement& stmt) {
        if (stmt.table.table_name.value.empty()) {
            return std::runtime_error("Delete statement missing target table");
        }

        if (!this->registry.has_table(stmt.table)) {
            return TableDoesntExist(stmt.table.table_name.value);
        }

        catalog::CppMetaTable table = this->registry.get_table(stmt.table.table_name);

        auto where_result = analyze_where(stmt.where, table);
        if (!where_result.is_valid) {
            return where_result.err.value();
        }
        
        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_table(const sql::CreateTableStatement& stmt) {
        if (exists_table((*db_name).c_str(), stmt.table.table_name.value.c_str())) {
            return TableExists(stmt.table.table_name.value);
        }
        return true;
    }

    AnalysisResult
    SemanticAnalyzer::validate_column_comparison(const std::unique_ptr<sql::AstNode>& left,
                                                 const std::unique_ptr<sql::AstNode>& right,
                                                 const catalog::CppMetaTable& table) {
        const sql::AstNode* column_node = nullptr;
        const sql::AstNode* value_node = nullptr;

        if (left->type == sql::AstNodeType::IDENTIFIER &&
            right->type == sql::AstNodeType::LITERAL) {
            column_node = left.get();
            value_node = right.get();
        } else if (right->type == sql::AstNodeType::IDENTIFIER &&
                   left->type == sql::AstNodeType::LITERAL) {
            column_node = right.get();
            value_node = left.get();
        } else {
            return std::runtime_error(
                "Invalid WHERE expression: comparison must be between column and literal");
        }

        const sql::SqlToken& column_token = std::get<sql::SqlToken>(column_node->value);
        const sql::SqlToken& value_token = std::get<sql::SqlToken>(value_node->value);

        sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
        if (!table.has_column(column_token.value)) {
            return ColumnDoesntExists(column_token.value);
        }

        const catalog::CppMetaColumn& column = table.get_column(column_token.value);

        if (!is_literal_assignable_to(literal_type, column.get_data_type())) {
            return std::runtime_error("Incompatible types conversion");
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::validate_column_assignment(const sql::AstNode& assignment, const catalog::CppMetaTable& table) {
        if (!std::holds_alternative<sql::BinaryExpr>(assignment.value)) {
            return std::runtime_error("Invalid assignment");
        }

        const sql::BinaryExpr& expr = std::get<sql::BinaryExpr>(assignment.value);
        if (expr.op != sql::AstOperator::ASSIGN) {
            return std::runtime_error("Invalid assignment: expected '='");
        }

        if (expr.left->type != sql::AstNodeType::IDENTIFIER ||
            expr.right->type != sql::AstNodeType::LITERAL)
            return std::runtime_error(
                "Invalid assignment: you can assign only literal to a identifier");

        const sql::AstNode* column_node = expr.left.get();
        const sql::AstNode* value_node = expr.right.get();

        const std::string& col_name = std::get<sql::SqlToken>(column_node->value).value;
        if (!table.has_column(col_name)) {
            return ColumnDoesntExists(std::string(col_name));
        }
        const catalog::CppMetaColumn& column = table.get_column(col_name);

        const sql::SqlToken& value_token = std::get<sql::SqlToken>(value_node->value);
        sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
        if (!is_literal_assignable_to(literal_type, column.get_data_type())) {
            return std::runtime_error("Incompatible types conversion in assignment");
        }

        return true;
    }

    AnalysisResult
    SemanticAnalyzer::analyze_where(const std::unique_ptr<sql::AstNode>& where,
                                    const catalog::CppMetaTable& table) {
        if (!where)
            return AnalysisResult(true);

        using sql::AstNodeType;
        using sql::AstOperator;

        if (where->type == AstNodeType::BINARY_EXPR) {
            const sql::BinaryExpr* expr = std::get_if<sql::BinaryExpr>(&where->value);
            if (!expr)
                return AnalysisResult(std::runtime_error("Invalid binary expression"));

            if (expr->op == AstOperator::ASSIGN)
                return AnalysisResult(std::runtime_error("Invalid condition operator"));

            if (expr->op == AstOperator::EQ || expr->op == AstOperator::NEQ ||
                expr->op == AstOperator::GR || expr->op == AstOperator::LT ||
                expr->op == AstOperator::GRE || expr->op == AstOperator::LTE) {

                if (!expr->left || !expr->right)
                    return std::runtime_error("Incomplete comparison expression");

                validate_column_comparison(expr->left, expr->right, table);
            }

            if (expr->op == AstOperator::AND || expr->op == AstOperator::OR) {
                analyze_where(expr->left, table);
                analyze_where(expr->right, table);
            }

            if (expr->op == AstOperator::NOT) {
                analyze_where(expr->right, table);
            }
        } else {
            return std::runtime_error("WHERE clause must be a logical or comparison expression");
        }

        return true;
    }

    void
    SemanticAnalyzer::ensure_db_exists(const std::string& name) {
        if (!exists_database(name.c_str())) {
            throw DbDoesntExists(name);
        }
    }

    void
    SemanticAnalyzer::ensure_db_exists() {
        ensure_db_exists(*this->db_name);
    }

    AnalysisResult
    SemanticAnalyzer::analyze_create_db(const sql::CreateDbStatement& stmt) {
        if (exists_database(stmt.name.value.c_str())) {
            return DbExists(stmt.name.value);
        }
        return true;
    }
} // namespace exe
