#include "include/planner.hpp"
#include "include/plan.hpp"
#include "../converter/include/converter.hpp"
#include "include/action.hpp"
#include "statement_converter.hpp"

namespace exe {
    QueryPlanner::QueryPlanner(engine::EngineConfig cfg, catalog::MetaRegistry& registry) 
        : cfg_(cfg), registry_(registry) {
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

        action.table = registry_.get_table(stmt.table.table_name.value, schema_name);

        if (stmt.where) {
            action.filter =
                std::make_unique<catalog::CppDataFilter>(converter::convert_binary_to_filter(
                    std::get<sql::BinaryExpr>(stmt.where->value), action.table
                ));
        }

        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::InsertStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        auto table = registry_.get_table(stmt.table.table_name.value, schema_name);
        auto schema = registry_.get_schema_by_id(table.schema_id);
        
        exe::InsertAction action;
        action.table = table;
        action.schema = schema;
        
        catalog::CppDataRow row;
        row.tokens.reserve(stmt.values.size());
        
        for (size_t i = 0; i < stmt.values.size() && i < table.columns.size(); i++) {
            const auto& value_token = stmt.values[i];
            const auto& column = table.columns[i];
            
            auto [data, size] = converter::convert_str_to_literal(value_token.value, column.data_type);
            catalog::CppDataToken cpp_token(size, static_cast<char*>(data), column.data_type);
            row.tokens.push_back(std::move(cpp_token));
        }
        
        action.row = std::move(row);
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::UpdateStatement& stmt) {
        std::string schema_name = stmt.table.schema_name.has_value()
                                      ? stmt.table.schema_name.value().value
                                      : cfg_.default_schema;

        auto table = registry_.get_table(stmt.table.table_name.value, schema_name);
        auto c_table = table.to_c();
        auto schema = registry_.get_schema_by_id(table.schema_id);
        
        auto row_update = converter::create_row_update(c_table, stmt);
        
        exe::UpdateByFilterAction action{
            .table = table,
            .schema = schema,
            .row_update = catalog::CppDataRowUpdate::from_c(row_update, table)
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

        auto table = registry_.get_table(stmt.table.table_name.value, schema_name);
        auto schema = registry_.get_schema_by_id(table.schema_id);
        
        exe::DeleteByFilterAction action;
        action.table = table;
        action.schema = schema;
        
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

        auto schema = registry_.get_schema(schema_name);
        
        exe::CreateTableAction action;
        action.schema = schema;
        
        catalog::CppMetaTable table;
        table.name = stmt.table.table_name.value;
        table.schema_id = schema.id;
        table.columns.reserve(stmt.columns.size());
        
        for (const auto& col_def : stmt.columns) {
            catalog::CppMetaColumn column;
            column.name = col_def.name.value;
            column.table_id = table.id;
            column.data_type = converter::convert_kw_to_dt(col_def.type.get_detail<sql::SqlKeyword>());
            column.flags = converter::convert_tokens_to_cfs(col_def.constraints);
            table.columns.push_back(std::move(column));
        }
        
        action.table = std::move(table);
        return SingleActionPlan{std::move(action)};
    }

    QueryPlan
    QueryPlanner::create_plan(const sql::CreateSchemaStatement& stmt) {
        exe::CreateSchemaAction action;
        
        catalog::CppMetaSchema schema;
        schema.name = stmt.name.value;
        
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