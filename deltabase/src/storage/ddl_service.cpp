//
// Created by poproshaikin on 6/11/26.
//

#include "ddl_service.hpp"

#include "wal_manager.hpp"

namespace storage
{
    using namespace types;

    DDLService::DDLService(
        const std::string& db_name,
        CatalogCache& catalog,
        wal::IWALManager& wal
    ) : db_name_(db_name), catalog_(catalog),
        wal_manager_(wal)
    {
    }

    MetaSchema*
    DDLService::create_schema(const std::string& schema_name, txn::Transaction& txn)
    {
        MetaSchema ms;
        ms.id = UUID::make();
        ms.name = schema_name;
        ms.db_name = db_name_;

        CreateSchemaRecord record(ms);
        txn.append_log(record);

        return catalog_.save_schema(ms, txn.get_id());
    }

    bool
    DDLService::exists_schema(const std::string& schema_name)
    {
        return catalog_.exists_schema(schema_name);
    }

    void
    DDLService::create_table(const std::string& table_name,
        const std::string& schema_name,
        const std::vector<types::ColumnDefinition>& columns,
        txn::Transaction& txn)
    {

    }


}