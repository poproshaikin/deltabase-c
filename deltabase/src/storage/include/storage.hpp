//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_STORAGE_HPP
#define DELTABASE_STORAGE_HPP
#include "../../types/include/catalog_snapshot.hpp"
#include "../../types/include/data_row.hpp"
#include "../../types/include/data_table.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../types/include/query_plan.hpp"
#include "../../types/include/transaction.hpp"

namespace storage
{
    class IStorage
    {
    public:
        virtual ~IStorage() = default;

        virtual const types::CatalogSnapshot
        get_catalog_snapshot() = 0;

        virtual bool
        needs_stream(types::IPlanNode& plan_node) = 0;

        virtual types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) = 0;

        virtual types::Transaction
        begin_txn() = 0;

        virtual uint64_t
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            types::DataRow& row,
            types::Transaction txn
        ) = 0;

        virtual void
        commit_txn(types::Transaction txn) = 0;
    };
}

#endif //DELTABASE_STORAGE_HPP