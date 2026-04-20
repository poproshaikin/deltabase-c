//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_EXECUTION_RESULT_HPP
#define DELTABASE_EXECUTION_RESULT_HPP
#include "data_row.hpp"
#include "data_table.hpp"
#include "scan_cursor.hpp"
#include "../../executor/include/node_executor.hpp"

namespace types
{
    class IExecutionResult
    {
    public:
        virtual ~IExecutionResult() = default;

        virtual bool
        next(DataRow& out) = 0;

        virtual OutputSchema
        output_schema() = 0;
    };

    class MaterializedResult final : public IExecutionResult
    {
        DataTable table_;
        uint64_t current_;
        OutputSchema schema_;

    public:
        MaterializedResult(const DataTable& table);

        bool
        next(DataRow& out) override;

        OutputSchema
        output_schema() override;
    };

    class StreamedResult final : public IExecutionResult
    {
        std::unique_ptr<exq::INodeExecutor> executor_;

    public:
        StreamedResult(std::unique_ptr<exq::INodeExecutor>&& executor);

        bool
        next(DataRow& out) override;

        OutputSchema
        output_schema() override;
    };

    class EmptyExecutionResult final : public IExecutionResult
    {
    public:
        bool
        next(DataRow& out) override;

        OutputSchema
        output_schema() override;
    };
}

#endif //DELTABASE_EXECUTION_RESULT_HPP