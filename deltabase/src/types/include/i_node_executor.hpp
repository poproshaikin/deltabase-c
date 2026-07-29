//
// Created by poproshaikin on 20.11.25.
//

#ifndef DELTABASE_I_NODE_EXECUTOR_HPP
#define DELTABASE_I_NODE_EXECUTOR_HPP

#include "data_row.hpp"
#include "data_table.hpp"

namespace exq
{
    class INodeExecutor
    {
    public:
        virtual ~INodeExecutor() = default;

        virtual void
        open() = 0;

        virtual bool
        next(types::DataRow& out) = 0;

        virtual void
        close() = 0;

        virtual types::OutputSchema
        output_schema() = 0;
    };
} // namespace exq

#endif // DELTABASE_I_NODE_EXECUTOR_HPP
