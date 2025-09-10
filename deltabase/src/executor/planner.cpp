#include "include/planner.hpp"
#include "include/plan.hpp"
#include "../converter/include/converter.hpp"
#include "../converter/include/statement_converter.hpp"
#include "include/action.hpp"

namespace exe {
    QueryPlanner::QueryPlanner(engine::EngineConfig cfg, storage::Storage& storage) 
        : cfg_(cfg), storage_(storage) {
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::AstNode& node) {

        if (std::holds_alternative<sql::BinaryExpr>(node.value) ||
            std::holds_alternative<sql::SqlToken>(node.value)) {
            throw std::runtime_error("Cannot create plan for BinaryExpr or SqlToken");
        }

        return std::visit(
            [this](const auto& a) -> QueryPlan { return this->create_plan(a); }, node.value
        );
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::SelectStatement& stmt) {

        exe::SeqScanAction action;
        action.columns.reserve(stmt.columns.size());
        
        for (const auto& col_token : stmt.columns) {
            action.columns.push_back(col_token.value);
        }

        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        action.table = storage_.get_table(stmt.table.table_name.value, schema_name);

        if (stmt.where) {
            action.filter = converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), action.table
            );
        }

        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::InsertStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        auto table = storage_.get_table(stmt.table.table_name.value, schema_name);
        auto schema = storage_.get_schema_by_id(table.schema_id);
        
        exe::InsertAction action;
        action.table = table;
        action.schema = schema;
        
        storage::DataRow row;
        row.tokens.reserve(stmt.values.size());
        
        for (size_t i = 0; i < stmt.values.size() && i < table.columns.size(); i++) {
            const auto& column = table.get_column(stmt.columns[i].value);
            const auto& value = stmt.values[i].value;
            
            storage::literal literal = converter::convert_str_to_literal(value, column.type);
            storage::DataToken token(literal, column.type);
            row.tokens.push_back(std::move(token));
        } 
        
        action.row = std::move(row);
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::UpdateStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        auto table = storage_.get_table(stmt.table.table_name.value, schema_name);
        auto schema = storage_.get_schema_by_id(table.schema_id);
        
        auto row_update = converter::create_row_update(table, stmt);
        
        exe::UpdateByFilterAction action{
            .table = table,
            .schema = schema,
            .row_update = row_update
        };

        if (stmt.where) {
            action.filter = converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), action.table
            );
        }
        
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::DeleteStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        exe::DeleteByFilterAction action;
        action.table  = storage_.get_table(stmt.table.table_name.value, schema_name);
        action.schema = storage_.get_schema_by_id(action.table.schema_id);
        
        if (stmt.where) {
            action.filter = converter::convert_binary_to_filter(
                std::get<sql::BinaryExpr>(stmt.where->value), action.table
            );
        }
        
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::CreateTableStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        exe::CreateTableAction action;
        action.schema = storage_.get_schema(schema_name);
        
        storage::MetaTable table;
        table.name = stmt.table.table_name.value;
        table.schema_id = action.schema.id;
        table.columns.reserve(stmt.columns.size());
        
        for (const auto& col_def : stmt.columns) {
            storage::MetaColumn column;
            column.name = col_def.name.value;
            column.table_id = table.id;
            column.type = converter::convert_kw_to_vt(col_def.type.get_detail<sql::SqlKeyword>());
            column.flags = converter::convert_tokens_to_cfs(col_def.constraints);
            table.columns.push_back(std::move(column));
        }
        
        action.table = std::move(table);
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::CreateSchemaStatement& stmt) {

        storage::MetaSchema schema;
        schema.name = stmt.name.value;

        exe::CreateSchemaAction action;
        action.schema = std::move(schema);

        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::CreateDbStatement& stmt) {
        exe::CreateDatabaseAction action;
        action.name = stmt.name.value;
        return SingleActionPlan{std::move(action)};
    }

    [[noreturn]] QueryPlan
    QueryPlanner::create_plan(const sql::SqlToken&) {
        throw std::runtime_error("Cannot create plan for SqlToken");
    }

    [[noreturn]] QueryPlan
    QueryPlanner::create_plan(const sql::BinaryExpr&) {
        throw std::runtime_error("Cannot create plan for BinaryExpr");
    }

    [[noreturn]] QueryPlan
    QueryPlanner::create_plan(const sql::ColumnDefinition&) {
        throw std::runtime_error("Cannot create plan for ColumnDefinition");
    }
}