//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_PLANNER_HPP
#define DELTABASE_PLANNER_HPP
#include "../../types/include/query_plan.hpp"

namespace exq
{
    class IPlanner
    {
    public:
        virtual ~IPlanner() = default;

        virtual types::QueryPlanNode
        plan(types::AstNode&& ast) = 0;
    };
}

#endif //DELTABASE_PLANNER_HPP