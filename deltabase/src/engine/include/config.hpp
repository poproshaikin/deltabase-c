#pragma once

#include <string>
#include <optional>

namespace engine {
    struct EngineConfig {
        std::optional<std::string> db_name;
        std::string default_schema = "public";

        // Default constructor
        EngineConfig() = default;

        // Constructor with db_name only
        explicit EngineConfig(std::string db_name) 
            : db_name(std::move(db_name)), default_schema("public") {
        }

        // Constructor with both parameters
        EngineConfig(std::string db_name, std::string default_schema)
            : db_name(std::move(db_name)), default_schema(std::move(default_schema)) {
        }
    };
}