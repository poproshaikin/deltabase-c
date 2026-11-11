//
// Created by poproshaikin on 11/11/26.
//

#ifndef DELTABASE_MEMORY_BUDGET_HPP
#define DELTABASE_MEMORY_BUDGET_HPP
#include "exceptions.hpp"

#include <cstdint>

namespace misc
{
    class MemoryBudget
    {
        uint64_t limit_;
        uint64_t used_ = 0;

    public:
        explicit
        MemoryBudget(uint64_t limit) : limit_(limit)
        {
        }

        // copying would split the accounting in two; moving carries it along
        MemoryBudget(const MemoryBudget&) = delete;
        MemoryBudget& operator=(const MemoryBudget&) = delete;
        MemoryBudget(MemoryBudget&&) = default;
        MemoryBudget& operator=(MemoryBudget&&) = default;

        void
        reserve(uint64_t bytes)
        {
            if (bytes > limit_ - used_)
                throw EngineException(
                    "Query exceeded its memory budget (" + std::to_string(used_ + bytes) +
                    " of " + std::to_string(limit_) + " bytes)",
                    EngineException::Code::MEMORY_BUDGET_EXCEEDED);

            used_ += bytes;
        }

        void
        release(uint64_t bytes)
        {
            if (bytes > used_)
                throw std::logic_error("MemoryBudget::release: released more than reserved");

            used_ -= bytes;
        }

        void
        reset(uint64_t new_limit)
        {
            limit_ = new_limit;
            used_ = 0;
        }
    };
}

#endif //DELTABASE_MEMORY_BUDGET_HPP