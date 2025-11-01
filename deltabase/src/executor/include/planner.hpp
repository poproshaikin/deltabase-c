#pragma once

#include "plan.hpp"
#include "../../sql/include/parser.hpp"
#include "../../engine/include/config.hpp"
#include "../../storage/include/storage.hpp"

namespace exe {
    class query_planner {
        engine::EngineConfig cfg_;
        storage::storage& storage_;

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
        query_planner(engine::EngineConfig cfg, storage::storage& storage);

        QueryPlan
        create_plan(const sql::AstNode& node);
    };  
}