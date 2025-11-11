//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_SIMPLE_PLANNER_HPP
#define DELTABASE_SIMPLE_PLANNER_HPP
#include "planner.hpp"

namespace exq
{
    class StdPlanner final : public IPlanner
    {


        types::QueryPlanNode
        plan_select(types::SelectStatement& stmt) const;

    public:
        types::QueryPlanNode
        plan(types::AstNode&& ast) override;
    };
}

#endif //DELTABASE_SIMPLE_PLANNER_HPP