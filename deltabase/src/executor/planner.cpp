#include "include/planner.hpp"
#include "include/plan.hpp"
#include "../converter/include/converter.hpp"
#include "../converter/include/statement_converter.hpp"
#include "include/action.hpp"

namespace exe {
    query_planner::query_planner(engine::EngineConfig cfg, storage::storage& storage) 
        : cfg_(cfg), storage_(storage) {
    }

    QueryPlan
    query_planner::create_plan(const sql::AstNode& node) {

        if (std::holds_alternative<sql::BinaryExpr>(node.value) ||
            std::holds_alternative<sql::SqlToken>(node.value)) {
            throw std::runtime_error("Cannot create plan for BinaryExpr or SqlToken");
        }

        return std::visit(
            [this](const auto& a) -> QueryPlan { return this->create_plan(a); }, node.value
        );
    }

    QueryPlan
    query_planner::create_plan(const sql::SelectStatement& stmt) {

        exe::SeqScanAction action{
            storage_.get_table(stmt.table)
        };

        action.columns.reserve(stmt.columns.size());

        for (const auto& col_token : stmt.columns) {
            action.columns.push_back(col_token.value);
        }

        if (stmt.where) {
            action.filter = converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), action.table
            );
        }

        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    query_planner::create_plan(const sql::InsertStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        auto& table = storage_.get_table(stmt.table.table_name.value, schema_name);
        auto& schema = storage_.get_schema_by_id(table.schema_id);

        exe::InsertAction action{.table = table, .schema = schema};

        storage::DataRow row;
        row.tokens.reserve(stmt.values.size());
        
        for (size_t i = 0; i < stmt.values.size() && i < table.columns.size(); i++) {
            const auto& column = table.get_column(stmt.columns[i].value);
            const auto& value = stmt.values[i].value;
            
            storage::bytes_v literal = converter::convert_str_to_literal(value, column.type);
            storage::DataToken token(literal, column.type);
            row.tokens.push_back(std::move(token));
        } 
        
        action.row = std::move(row);
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    query_planner::create_plan(const sql::UpdateStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        exe::UpdateByFilterAction action{
            .table = storage_.get_table(stmt.table.table_name.value, schema_name),
            .schema = storage_.get_schema_by_id(action.table.schema_id),
            .row_update = converter::create_row_update(action.table, stmt)
        };

        if (stmt.where) {
            action.filter = converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), action.table
            );
        }
        
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    query_planner::create_plan(const sql::DeleteStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        auto& table = storage_.get_table(stmt.table.table_name.value, schema_name);

        exe::DeleteByFilterAction action {
            .schema = storage_.get_schema_by_id(table.schema_id),
            .table = table
        };

        if (stmt.where) {
            action.filter = converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), action.table
            );
        }
        
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    query_planner::create_plan(const sql::CreateTableStatement& stmt) {
        auto& schema = storage_.get_schema(stmt.table);

        storage::MetaTable table;
        table.name = stmt.table.table_name.value;
        table.schema_id = schema.id;
        table.columns.reserve(stmt.columns.size());
        
        for (const auto& col_def : stmt.columns) {
            storage::meta_column column;
            column.name = col_def.name.value;
            column.table_id = table.id;
            column.type = converter::convert_kw_to_vt(col_def.type.get_detail<sql::SqlKeyword>());
            column.flags = converter::convert_tokens_to_cfs(col_def.constraints);
            table.columns.push_back(std::move(column));
        }

        return SingleActionPlan{exe::CreateTableAction{
            .schema = schema,
            .table = table,
        }};
    }

    QueryPlan
    query_planner::create_plan(const sql::CreateSchemaStatement& stmt) {
        storage::MetaSchema schema;
        schema.name = stmt.name.value;

        return SingleActionPlan{exe::CreateSchemaAction{schema}};
    }

    QueryPlan
    query_planner::create_plan(const sql::CreateDbStatement& stmt) {
        exe::CreateDatabaseAction action;
        action.name = stmt.name.value;
        return SingleActionPlan{std::move(action)};
    }

    [[noreturn]] QueryPlan
    query_planner::create_plan(const sql::SqlToken&) {
        throw std::runtime_error("Cannot create plan for SqlToken");
    }

    [[noreturn]] QueryPlan
    query_planner::create_plan(const sql::BinaryExpr&) {
        throw std::runtime_error("Cannot create plan for BinaryExpr");
    }

    [[noreturn]] QueryPlan
    query_planner::create_plan(const sql::ColumnDefinition&) {
        throw std::runtime_error("Cannot create plan for ColumnDefinition");
    }
}