//
// Created by poproshaikin on 6/19/26.
//

#include "constraint_enforcer.hpp"

namespace storage
{
    ConstraintEnforcer::ConstraintEnforcer(DqlService & dql) : dql_(dql)
    {
    }

    void
    ConstraintEnforcer::validate_or_throw(
        const types::MetaTable & mt,
        const std::vector<types::DataToken> & row,
        txn::Transaction & txn)
    {
        // TODO: NOT NULL on PK columns
        // TODO: UNIQUE / PRIMARY KEY (via index lookup through dql_)
        // TODO: FK existence check (via dql_.seq_scan / index lookup)
    }
}
