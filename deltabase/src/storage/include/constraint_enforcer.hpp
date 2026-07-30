//
// Created by poproshaikin on 6/19/26.
//

#ifndef DELTABASE_CONSTRAINT_ENFORCER_HPP
#define DELTABASE_CONSTRAINT_ENFORCER_HPP

#include "dql_service.hpp"
#include "dml_service.hpp"
#include "catalog.hpp"
#include "../../types/include/data_token.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../transactions/include/transaction.hpp"

#include <vector>

namespace storage
{
    class ConstraintEnforcer
    {
        DQLService& dql_;
        DMLService& dml_;

        CatalogCache& catalog_;

        void
        handle_fk_on_delete(
            const types::MetaTable& deleted_from_table,
            types::MetaTable& current_table,
            const types::MetaForeignKeyConstraint& fk,
            const types::DataRow& deleting_row,
            const types::MetaColumn& col,
            txn::Transaction* txn);

    public:
        explicit ConstraintEnforcer(DQLService& dql, DMLService& dml, CatalogCache& catalog);

        void
        validate_or_throw(
            const types::MetaTable& mt,
            const std::vector<types::DataToken>& row) const;

        void
        on_delete(const types::MetaTable& mt, const types::DataRow& deleting_row, txn::Transaction* txn);
    };
}

#endif //DELTABASE_CONSTRAINT_ENFORCER_HPP
