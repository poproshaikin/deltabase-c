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

    StreamedResult::StreamedResult(std::unique_ptr<exq::INodeExecutor>&& executor)
        : executor_(std::move(executor))
    {
    }

    StreamedResult::StreamedResult(std::unique_ptr<exq::INodeExecutor>&& executor, std::function<void()> on_exhausted)
        : executor_(std::move(executor)), on_exhausted_(std::move(on_exhausted))
    {
    }

    bool
    StreamedResult::next(DataRow& out)
    {
        if (executor_->next(out))
            return true;

        if (on_exhausted_)
        {
            on_exhausted_();
            on_exhausted_ = nullptr;
        }
        return false;
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
} // namespace types