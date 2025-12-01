//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_DB_CFG_HPP
#define DELTABASE_DB_CFG_HPP
#include <string>

namespace types
{
    struct Config
    {
        enum class IoType
        {
            File = 1,
        };

        enum class PlannerType
        {
            Std = 1
        };

        enum class SerializerType
        {
            Std = 1
        };

        std::string name;
        std::string default_schema = "common";

        std::filesystem::path db_path;

        IoType io_type = IoType::File;
        PlannerType planner_type = PlannerType::Std;
        SerializerType serializer_type = SerializerType::Std;

        Config() = default;
        Config(const std::string& name) : name(name)
        {
        }
    };
}

#endif //DELTABASE_DB_CFG_HPP