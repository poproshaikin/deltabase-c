//
// Created by poproshaikin on 01.12.25.
//

#ifndef DELTABASE_PLANNER_FACTORY_HPP
#define DELTABASE_PLANNER_FACTORY_HPP
#include "planner.hpp"
#include "../../types/include/config.hpp"
#include "../../storage/include/db_instance.hpp"

#include <memory>

namespace exq
{
    class PlannerFactory
    {
    public:
        std::unique_ptr<IPlanner>
        make_planner(const types::Config& config, storage::IDbInstance& db);
    };
}

#endif //DELTABASE_PLANNER_FACTORY_HPP