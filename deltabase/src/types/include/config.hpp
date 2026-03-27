//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_DB_CFG_HPP
#define DELTABASE_DB_CFG_HPP
#include "../../misc/include/static_storage.hpp"
#include "data_page.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace engine
{
    class Engine;
}

namespace types
{
    struct Config
    {
        enum class IoType
        {
            File = 1,
            DetachedFile
        };

        enum class PlannerType
        {
            Std = 1
        };

        enum class SerializerType
        {
            Std = 1
        };

    private:
        friend class engine::Engine;

        Config() = default;

        explicit Config(
            const std::filesystem::path& db_path,
            IoType io,
            PlannerType planner,
            SerializerType serializer
        )
            : db_name(std::nullopt), db_path(db_path), io_type(io), planner_type(planner),
              serializer_type(serializer)
        {
        }

        explicit Config(
            const std::string& name,
            const std::filesystem::path& db_path,
            IoType io,
            PlannerType planner,
            SerializerType serializer
        )
            : db_name(name), db_path(db_path), io_type(io), planner_type(planner),
              serializer_type(serializer)
        {
        }

    public:

        std::optional<std::string> db_name = std::nullopt;
        std::string default_schema = "common";

        std::filesystem::path db_path = {};

        IoType io_type;
        PlannerType planner_type;
        SerializerType serializer_type;

        LSN last_checkpoint_lsn = 0;

        static Config
        detached()
        {
            return Config(
                misc::StaticStorage::get_executable_path() / "data",
                IoType::DetachedFile,
                PlannerType::Std,
                SerializerType::Std
            );
        }

        static Config
        std(const std::string& db_name)
        {
            return Config(
                db_name,
                misc::StaticStorage::get_executable_path() / "data",
                IoType::File,
                PlannerType::Std,
                SerializerType::Std
            );
        }
    };
} // namespace types

#endif // DELTABASE_DB_CFG_HPP