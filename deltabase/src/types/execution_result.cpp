//
// Created by poproshaikin on 24.11.25.
//

#include "include/execution_result.hpp"

namespace types
{
    MaterializedResult::MaterializedResult(const DataTable& table)
        : table_(table), schema_(table.output_schema), current_(0)
    {
    }

    bool
    MaterializedResult::next(DataRow& out)
    {
        if (current_ >= table_.rows.size())
            return false;

        out = table_.rows[current_++];
        return true;
    }

    OutputSchema
    MaterializedResult::output_schema()
    {
        return schema_;
    }

    StreamedResult::StreamedResult(std::unique_ptr<exq::INodeExecutor>&& executor) : executor_(std::move(executor))
    {
    }

    bool
    StreamedResult::next(DataRow& out)
    {
        return executor_->next(out);
    }

    OutputSchema
    StreamedResult::output_schema()
    {
        return executor_->output_schema();
    }

    bool
    EmptyExecutionResult::next(DataRow& out)
    {
        return false;
    }

    OutputSchema
    EmptyExecutionResult::output_schema()
    {
        return {};
    }

    bool
    EmptyExecutionResult::is_exhausted()
    {
        return true;
    }
} // namespace types