//
// Created by poproshaikin on 01.12.25.
//

#include "planner_factory.hpp"

#include "../storage/include/std_db_instance.hpp"
#include "std_planner.hpp"

namespace exq
{
    using namespace types;

    std::unique_ptr<IPlanner>
    PlannerFactory::make_planner(const Config& config, storage::IDbInstance& db)
    {
        switch (config.planner_type)
        {
        case Config::PlannerType::Std:
            return std::make_unique<StdPlanner>(StdPlanner(config, db));
        default:
            throw std::runtime_error(
                "PlannerFactory::make_planner: Invalid planner type " + std::to_string(
                    static_cast<int>(config.planner_type)
                )
            );
        }
    }
}