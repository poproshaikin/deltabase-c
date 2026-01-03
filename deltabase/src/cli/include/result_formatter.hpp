//
// Created by poproshaikin on 02.01.26.
//

#ifndef DELTABASE_RESULTFORMATTER_HPP
#define DELTABASE_RESULTFORMATTER_HPP
#include "execution_result.hpp"

#include <string>

namespace cli
{
    class ResultFormatter
    {
    public:
        std::string
        format(const types::IExecutionResult& result);
    };
}

#endif //DELTABASE_RESULTFORMATTER_HPP