//
// Created by poproshaikin on 10.11.25.
//

#ifndef DELTABASE_DATABASE_HPP
#define DELTABASE_DATABASE_HPP

#include "../../types/include/execution_result.hpp"

#include <memory>
#include <string>

namespace engine
{
    class IDatabase
    {
    public:
        virtual ~IDatabase() = default;

        virtual std::unique_ptr<types::IExecutionResult>
        execute(const std::string& query) = 0;
    };
}

#endif //DELTABASE_DATABASE_HPP