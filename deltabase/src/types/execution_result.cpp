//
// Created by poproshaikin on 24.11.25.
//

#include "include/execution_result.hpp"

namespace types
{
    MaterializedResult::MaterializedResult(const DataTable& table)
        : table_(table), current_(0)
    {
    }

    bool
    MaterializedResult::next(DataRow& out)
    {
        if (current_++ >= table_.rows.size())
            return false;

        out = table_.rows[current_];
        return true;
    }
}