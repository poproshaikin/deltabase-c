//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_STORAGE_HPP
#define DELTABASE_STORAGE_HPP
#include "../../types/include/data_row.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../types/include/meta_schema.hpp"
#include "../../types/include/query_plan.hpp"
#include "../../types/include/transaction.hpp"

namespace storage
{
    class IStorage
    {
    public:
        virtual ~IStorage() = default;

        virtual std::shared_ptr<types::MetaTable>
        get_table(std::string table_name, std::string schema_name) = 0;

        virtual std::shared_ptr<types::MetaTable>
        get_table(types::TableIdentifier identifier) = 0;

        virtual std::shared_ptr<types::MetaSchema>
        get_schema(std::string schema_name) = 0;

        virtual uint64_t
        insert_row(types::MetaTable& table, const types::DataRow& row) = 0;

        virtual uint64_t
        insert_row(types::MetaTable& table, const types::DataRow& row, types::Transaction txn) = 0;

        bool
        needs_stream(types::IPlanNode& plan_node);
    };
}

#endif //DELTABASE_STORAGE_HPP