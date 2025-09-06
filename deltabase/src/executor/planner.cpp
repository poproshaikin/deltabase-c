#include "include/planner.hpp"
#include "include/plan.hpp"
#include "converter.hpp"
#include "include/action.hpp"

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
        auto c_table = action.table.to_c();

        if (stmt.where) {
            action.filter =
                std::make_unique<catalog::CppDataFilter>(converter::convert_binary_to_filter(
                    std::get<sql::BinaryExpr>(stmt.where->value), c_table
                ));
        }

        catalog::CppMetaTable::cleanup_c(c_table);
        return SingleActionPlan{std::move(action)};
    }
}