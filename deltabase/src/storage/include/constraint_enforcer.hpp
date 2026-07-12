//
// Created by poproshaikin on 6/19/26.
//

#ifndef DELTABASE_CONSTRAINT_ENFORCER_HPP
#define DELTABASE_CONSTRAINT_ENFORCER_HPP

#include "dql_service.hpp"
#include "../../types/include/data_token.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../transactions/include/transaction.hpp"

#include <vector>

namespace storage
{
    class ConstraintEnforcer
    {
        DqlService& dql_;

    public:
        explicit ConstraintEnforcer(DqlService& dql);

        void
        validate_or_throw(
            const types::MetaTable& mt,
            const std::vector<types::DataToken>& row,
            txn::Transaction& txn);
    };
}

#endif //DELTABASE_CONSTRAINT_ENFORCER_HPP
