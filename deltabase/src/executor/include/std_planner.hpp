//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_SIMPLE_PLANNER_HPP
#define DELTABASE_SIMPLE_PLANNER_HPP
#include "planner.hpp"
#include "storage.hpp"
#include "../../types/include/db_cfg.hpp"

namespace exq
{
    class StdPlanner final : public IPlanner
    {
        storage::IStorage& storage_;
        const types::DbConfig& db_config_;

        types::QueryPlan
        plan_select(types::SelectStatement& stmt) const;

        types::QueryPlan
        plan_insert(types::InsertStatement& stmt) const;

        types::QueryPlan
        plan_update(types::UpdateStatement& stmt) const;

        types::QueryPlan
        plan_delete(types::DeleteStatement& stmt) const;

    public:
        StdPlanner(const types::DbConfig& db_config, storage::IStorage& storage);

        types::QueryPlan
        plan(types::AstNode&& ast) override;
    };
}

#endif //DELTABASE_SIMPLE_PLANNER_HPP