//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_ENGINE_HPP
#define DELTABASE_ENGINE_HPP

#include "database.hpp"

#include <filesystem>

namespace engine
{
    class Engine
    {
        std::filesystem::path data_path_;
        std::unique_ptr<IDatabase> db_;

    public:
        explicit
        Engine(const std::filesystem::path& data_path);

        void
        attach_db(const std::string& db_name);

        void
        create_db(const std::string& db_name);

        std::unique_ptr<types::IExecutionResult>
        execute_query(const std::string& query);
    };
}

#endif //DELTABASE_ENGINE_HPP