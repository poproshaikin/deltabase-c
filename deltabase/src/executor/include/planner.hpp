#pragma once

#include "plan.hpp"
#include "../../sql/include/parser.hpp"
#include "../../engine/include/config.hpp"
#include "../../storage/include/storage.hpp"

namespace exe {
    class QueryPlanner {
        engine::EngineConfig cfg_;
        storage::Storage& storage_;

        QueryPlan
        create_plan(const sql::SelectStatement& stmt);
        QueryPlan
        create_plan(const sql::InsertStatement& stmt);
        QueryPlan
        create_plan(const sql::UpdateStatement& stmt);
        QueryPlan
        create_plan(const sql::DeleteStatement& stmt);
        QueryPlan
        create_plan(const sql::CreateTableStatement& stmt);
        QueryPlan
        create_plan(const sql::CreateSchemaStatement& stmt);
        QueryPlan
        create_plan(const sql::CreateDbStatement& stmt);

        [[noreturn]] QueryPlan
        create_plan(const sql::SqlToken&);
        [[noreturn]] QueryPlan
        create_plan(const sql::BinaryExpr&);
        [[noreturn]] QueryPlan
        create_plan(const sql::ColumnDefinition&);

    public:
        QueryPlanner(engine::EngineConfig cfg, storage::Storage& storage);

        QueryPlan
        create_plan(const sql::AstNode& node);
    };  
}