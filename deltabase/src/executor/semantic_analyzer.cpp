#include "include/semantic_analyzer.hpp"
#include <variant>
#include <stdexcept>
#include <string.h>

extern "C" {
    #include "../core/include/core.h"
}

namespace exe {
    SemanticAnalyzer::SemanticAnalyzer(std::string db_name) : _db_name(db_name) { }

    void SemanticAnalyzer::analyze(const sql::AstNode* ast) {
        switch (ast->type) {
            case sql::AstNodeType::SELECT:
                analyze_select(std::get<sql::SelectStatement>(ast->value));
                break;
            case sql::AstNodeType::INSERT:
                analyze_insert(std::get<sql::InsertStatement>(ast->value));
                break;
            case sql::AstNodeType::UPDATE:
                analyze_update(std::get<sql::UpdateStatement>(ast->value));
                break;
            case sql::AstNodeType::DELETE:
                analyze_delete(std::get<sql::DeleteStatement>(ast->value));
                break;
            default:
                throw std::runtime_error("Unsupported AST node type for semantic analysis");
        }
    }

    void SemanticAnalyzer::analyze_select(const sql::SelectStatement& selectStmt) { 
        if (selectStmt.table.value.empty()) {
            throw std::runtime_error("Select statement missing target table");
        }

        MetaTable schema;
        const sql::SqlToken& table = selectStmt.table;
        if (get_table(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error("Table doesn't exists");
        }

        for (const sql::SqlToken& col : selectStmt.columns) {
            const char *name = col.value.data();
            ensure_column_exists(&schema, name);
        }

        analyze_where(selectStmt.where, &schema);
    }

    void SemanticAnalyzer::analyze_insert(const sql::InsertStatement& insertStmt) {
        if (insertStmt.table.value.empty()) {
            throw std::runtime_error("Insert statement missing target table");
        }
        if (insertStmt.columns.size() != 0 && insertStmt.columns.size() != insertStmt.values.size()) {
            throw std::runtime_error("Insert statement columns count does not match values count");
        }

        MetaTable schema;
        const sql::SqlToken& table = insertStmt.table;
        if (get_table(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error("Table doesn't exists");
        }

        for (size_t i = 0; i < insertStmt.columns.size(); i++) {
            const char *name = insertStmt.columns[i].value.data();
            ensure_column_exists(&schema, name);

            const sql::SqlToken& value_token = insertStmt.values[i];
            const sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
            MetaColumn* column = find_column(name, &schema);
            if (!column) {
                throw std::runtime_error("Column doesn't exist");
            }

            if (!is_literal_compatible(literal_type, column->data_type)) {
                throw std::runtime_error("Incompatible types conversion");
            }
        }
    }

    void SemanticAnalyzer::analyze_update(const sql::UpdateStatement& updateStmt) {
        if (updateStmt.table.value.empty()) {
            throw std::runtime_error("Update statement missing target table");
        }
        if (updateStmt.assignments.empty()) {
            throw std::runtime_error("Update statement missing assignments");
        }

        MetaTable schema;
        const sql::SqlToken& table = updateStmt.table;
        if (get_table(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error("Table doesn't exists");
        }

        for (size_t i = 0; i < updateStmt.assignments.size(); i++) {
            validate_column_assignment(const_cast<sql::AstNode*>(&updateStmt.assignments[i]), &schema);
        }

        analyze_where(updateStmt.where, &schema);
    }

    void SemanticAnalyzer::analyze_delete(const sql::DeleteStatement& deleteStmt) {
        if (deleteStmt.table.value.empty()) {
            throw std::runtime_error("Delete statement missing target table");
        }

        MetaTable schema;
        const sql::SqlToken& table = deleteStmt.table;
        if (get_table(_db_name.data(), table.value.data(), &schema) != 0) {
            throw std::runtime_error("Table doesn't exist");
        }

        analyze_where(deleteStmt.where, &schema);
    }

    void SemanticAnalyzer::analyze_where(const std::unique_ptr<sql::AstNode>& where, const MetaTable* table) {
        if (!where) return;

        using sql::AstNodeType;
        using sql::AstOperator;

        if (where->type == AstNodeType::BINARY_EXPR) {
            const sql::BinaryExpr* expr = std::get_if<sql::BinaryExpr>(&where->value);
            if (!expr) throw std::runtime_error("Invalid binary expression");

            if (expr->op == AstOperator::ASSIGN) 
                throw std::runtime_error("Invalid condition operator");

            if (expr->op == AstOperator::EQ || expr->op == AstOperator::NEQ ||
                    expr->op == AstOperator::GR || expr->op == AstOperator::LT ||
                    expr->op == AstOperator::GRE || expr->op == AstOperator::LTE) {

                if (!expr->left || !expr->right)
                    throw std::runtime_error("Incomplete comparison expression");

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
            throw std::runtime_error("WHERE clause must be a logical or comparison expression");
        }
    }

    void SemanticAnalyzer::validate_column_comparison(
            const std::unique_ptr<sql::AstNode>& left,
            const std::unique_ptr<sql::AstNode>& right,
            const MetaTable* table
            ) {
        const sql::AstNode* column_node = nullptr;
        const sql::AstNode* value_node = nullptr;

        if (left->type == sql::AstNodeType::IDENTIFIER && right->type == sql::AstNodeType::LITERAL) {
            column_node = left.get();
            value_node = right.get();
        } else if (right->type == sql::AstNodeType::IDENTIFIER && left->type == sql::AstNodeType::LITERAL) {
            column_node = right.get();
            value_node = left.get();
        } else {
            throw std::runtime_error("Invalid WHERE expression: comparison must be between column and literal");
        }


        const sql::SqlToken& column_token = std::get<sql::SqlToken>(column_node->value);
        const sql::SqlToken& value_token = std::get<sql::SqlToken>(value_node->value);

        sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
        MetaColumn* column = find_column(column_token.value.data(), table);
        if (!column) {
            throw std::runtime_error("Column was not found");
        }

        if (!is_literal_compatible(literal_type, column->data_type)) {
            throw std::runtime_error("Incompatible types conversion"); 
        }

        bool found = false;

        for (uint64_t i = 0; i < table->columns_count; ++i) {
            MetaColumn* col = table->columns[i];
            if (column_token.value == col->name) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("Unknown column in WHERE clause: " + column_token.value);
        }
    }

    void SemanticAnalyzer::validate_column_assignment(sql::AstNode* assignment, MetaTable *table) {
        sql::BinaryExpr* expr = std::get_if<sql::BinaryExpr>(&assignment->value);
        if (!expr) {
            throw std::runtime_error("Invalid assignment");
        }   
        if (expr->op != sql::AstOperator::ASSIGN) {
            throw std::runtime_error("Invalid assignment: expected '='");
        }

        if (expr->left->type != sql::AstNodeType::IDENTIFIER || expr->right->type != sql::AstNodeType::LITERAL) 
            throw std::runtime_error("Invalid assignment: you can assign only literal to a identifier");
        
        const sql::AstNode* column_node = expr->left.get();
        const sql::AstNode* value_node = expr->right.get();

        const char *col_name = std::get<sql::SqlToken>(column_node->value).value.data();
        MetaColumn* column = find_column(col_name, table);
        if (!column) {
            throw std::runtime_error("Column doesn't exist");
        }

        const sql::SqlToken& value_token = std::get<sql::SqlToken>(value_node->value);
        sql::SqlLiteral literal_type = std::get<sql::SqlLiteral>(value_token.detail);
        if (!is_literal_compatible(literal_type, column->data_type)) {
            throw std::runtime_error("Incompatible types conversion in assignment");
        }
    }

    MetaColumn* SemanticAnalyzer::ensure_column_exists(const MetaTable* table, std::string colname) {
        bool found = false;
        MetaColumn* col = nullptr;
        for (size_t i = 0; i < table->columns_count; i++) {
            if (table->columns[i]->name == colname) {
                found = true;
                col = table->columns[i];
                break;
            }
        }
        if (!found) {
            throw std::runtime_error("Column was not found");
        }
        return col;
    }

}
