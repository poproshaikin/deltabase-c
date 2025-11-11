//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_EXECUTION_RESULT_HPP
#define DELTABASE_EXECUTION_RESULT_HPP
#include "data_row.hpp"

namespace types
{
    class IExecutionResult
    {
    public:
        virtual ~IExecutionResult() = default;

        virtual bool
        next() = 0;

        virtual DataRow
        get() = 0;
    };
}

#endif //DELTABASE_EXECUTION_RESULT_HPP