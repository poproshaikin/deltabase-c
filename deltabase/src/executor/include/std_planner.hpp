//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_SIMPLE_PLANNER_HPP
#define DELTABASE_SIMPLE_PLANNER_HPP
#include "planner.hpp"
#include "../../types/include/db_cfg.hpp"
#include "../../storage/include/db_instance.hpp"

namespace exq
{
    class StdPlanner final : public IPlanner
    {
        storage::IDbInstance& db_;
        const types::Config& db_config_;

        types::QueryPlan
        plan_select(types::SelectStatement& stmt) const;

        types::QueryPlan
        plan_insert(types::InsertStatement& stmt) const;

        types::QueryPlan
        plan_update(types::UpdateStatement& stmt) const;

        types::QueryPlan
        plan_delete(types::DeleteStatement& stmt) const;

        types::QueryPlan
        plan_create_db(types::CreateDbStatement& stmt) const;

    public:
        explicit
        StdPlanner(const types::Config& db_config, storage::IDbInstance& db);

        types::QueryPlan
        plan(types::AstNode&& ast) override;
    };
}

#endif //DELTABASE_SIMPLE_PLANNER_HPP