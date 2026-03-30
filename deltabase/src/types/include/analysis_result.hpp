//
// Created by poproshaikin on 03.12.25.
//

#ifndef DELTABASE_ANALYSIS_RESULT_HPP
#define DELTABASE_ANALYSIS_RESULT_HPP
#include <optional>
#include <stdexcept>

namespace types
{
    struct AnalysisResult
    {
        bool is_valid;

        std::optional<std::runtime_error> err;

        std::optional<bool> is_system_query;

        explicit
        AnalysisResult(bool is_valid) : is_valid(is_valid), is_system_query(false)
        {
        }

        explicit
        AnalysisResult(bool is_valid, bool is_system_query)
            : is_valid(is_valid),
              is_system_query(is_system_query)
        {
        }

        explicit
        AnalysisResult(std::runtime_error err) : is_valid(false), err(err)
        {
        }

        explicit
        AnalysisResult(std::runtime_error err, bool is_system_query)
            : is_valid(false), err(err), is_system_query(is_system_query)
        {
        }

        explicit
        AnalysisResult(bool is_valid,
                       std::optional<std::runtime_error> err,
                       std::optional<bool> is_system_query)
            : is_valid(is_valid), err(std::move(err)), is_system_query(is_system_query)
        {
        }
    };
}

#endif //DELTABASE_ANALYSIS_RESULT_HPP