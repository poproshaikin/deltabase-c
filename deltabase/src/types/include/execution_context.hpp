//
// Created by poproshaikin on 6/7/26.
//

#ifndef DELTABASE_EXECUTION_CONTEXT_HPP
#define DELTABASE_EXECUTION_CONTEXT_HPP
#include "../../transactions/include/transaction.hpp"

namespace types
{
    struct ExecutionContext
    {
        txn::Transaction* txn;
    };
}

#endif //DELTABASE_EXECUTION_CONTEXT_HPP
