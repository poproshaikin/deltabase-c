//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_EXECUTION_RESULT_HPP
#define DELTABASE_EXECUTION_RESULT_HPP
#include "data_row.hpp"
#include "data_table.hpp"

namespace types
{
    class IExecutionResult
    {
    public:
        virtual ~IExecutionResult() = default;

        virtual bool
        next(DataRow& out) = 0;
    };

    class MaterializedResult final : public IExecutionResult
    {
        DataTable table_;
        uint64_t current_;
    public:
        MaterializedResult(const DataTable& table);

        bool
        next(DataRow& out) override;
    };
}

#endif //DELTABASE_EXECUTION_RESULT_HPP