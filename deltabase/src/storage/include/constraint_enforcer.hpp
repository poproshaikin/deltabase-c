//
// Created by poproshaikin on 6/19/26.
//

#ifndef DELTABASE_CONSTRAINT_ENFORCER_HPP
#define DELTABASE_CONSTRAINT_ENFORCER_HPP

#include "dql_service.hpp"
#include "catalog.hpp"
#include "../../types/include/data_token.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../transactions/include/transaction.hpp"

#include <vector>

namespace storage
{
    class ConstraintEnforcer
    {
        DqlService& dql_;
        const CatalogCache& catalog_;

    public:
        explicit ConstraintEnforcer(DqlService& dql, const CatalogCache& catalog);

        void
        validate_or_throw(
            const types::MetaTable& mt,
            const std::vector<types::DataToken>& row);
    };
}

#endif //DELTABASE_CONSTRAINT_ENFORCER_HPP
