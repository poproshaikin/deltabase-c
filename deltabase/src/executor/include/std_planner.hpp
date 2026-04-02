//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_SIMPLE_PLANNER_HPP
#define DELTABASE_SIMPLE_PLANNER_HPP
#include "planner.hpp"
#include "../../types/include/config.hpp"
#include "../../storage/include/db_instance.hpp"

namespace exq
{
    class StdPlanner final : public IPlanner
    {
        storage::IDbInstance& db_;
        types::Config db_config_;

        types::QueryPlan
        plan(types::SelectStatement& stmt) const;

        types::QueryPlan
        plan(types::InsertStatement& stmt) const;

        types::QueryPlan
        plan(types::UpdateStatement& stmt) const;

        types::QueryPlan
        plan(types::DeleteStatement& stmt) const;

        types::QueryPlan
        plan(types::CreateDbStatement& stmt) const;

        types::QueryPlan
        plan(const types::CreateTableStatement& table) const;

        types::QueryPlan
        plan(const types::CreateIndexStatement& stmt) const;

        types::QueryPlan
        plan(const types::DropIndexStatement& stmt) const;

    public:
        explicit
        StdPlanner(const types::Config& db_config, storage::IDbInstance& db);

        types::QueryPlan
        plan(types::AstNode&& ast) override;
    };
}

#endif //DELTABASE_SIMPLE_PLANNER_HPP