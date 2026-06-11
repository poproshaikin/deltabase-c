//
// Created by poproshaikin on 6/11/26.
//

#ifndef DELTABASE_DDL_SERVICE_HPP
#define DELTABASE_DDL_SERVICE_HPP
#include "catalog.hpp"
#include "../../transactions/include/transaction.hpp"
#include "wal_manager.hpp"

namespace storage
{
    class DDLService
    {
        std::string db_name_;
        CatalogCache& catalog_;
        wal::IWALManager& wal_manager_;

    public:
        explicit DDLService(const std::string& db_name, CatalogCache& catalog, wal::IWALManager& wal_manager);

        types::MetaSchema*
        create_schema(const std::string& schema_name, txn::Transaction& txn);

        bool
        exists_schema(const std::string& schema_name);

        void
        create_table(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::ColumnDefinition>& columns,
            txn::Transaction& txn);

        bool
        exists_table(const std::string& table_name, const std::string& schema_name);

        bool
        exists_table(const types::TableIdentifier& identifier);

        void
        drop_table(
            const std::string& table_name,
            const std::string& schema_name,
            txn::Transaction& txn);
    };
}

#endif //DELTABASE_DDL_SERVICE_HPP
