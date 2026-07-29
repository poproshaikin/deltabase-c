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
        types::Config& cfg_;
        CatalogCache& catalog_;
        BufferPool& buffer_pool_;
        wal::IWALManager& wal_manager_;
        IIOManager& io_manager_;

        types::MetaColumn
        resolve_column(const types::ColumnDefinition& column_def, const types::MetaTable& mt);

        types::DataRow
        extend_row(const types::DataRow& old_row, const types::MetaColumn& new_column);

    public:
        explicit
        DDLService(
            types::Config& cfg,
            BufferPool& buffer_pool,
            CatalogCache& catalog,
            wal::IWALManager& wal_manager,
            IIOManager& io_manager);

        types::MetaSchema*
        create_schema(const std::string& schema_name, txn::Transaction& txn);

        bool
        exists_schema(const std::string& schema_name);

        types::MetaSchema*
        get_schema(const std::string& name);

        types::MetaTable*
        create_table(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::ColumnDefinition>& columns,
            txn::Transaction& txn);

        bool
        exists_table(const std::string& table_name, const std::string& schema_name);

        bool
        exists_table(const types::TableIdentifier& identifier);

        types::MetaTable*
        get_table(const std::string& table_name, const std::string& schema_name);

        types::MetaTable*
        get_table(const types::TableIdentifier& identifier);

        void
        drop_table(
            const std::string& table_name,
            const std::string& schema_name,
            txn::Transaction& txn);

        void
        add_column(
            const std::string& table_name,
            const std::string& schema_name,
            const types::ColumnDefinition& column,
            txn::Transaction& txn);

        types::UUID
        create_sequence(
            const std::string& sequence_name,
            const std::string& schema_name,
            txn::Transaction& txn);

        void
        create_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& column_name,
            const std::string& schema_name,
            bool is_unique,
            txn::Transaction& txn);

        bool
        exists_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name);

        bool
        exists_index(
            const std::string& index_name,
            const types::TableIdentifier& table_identifier);

        types::MetaIndex*
        get_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name);

        types::MetaIndex*
        get_index(
            const std::string& index_name,
            const types::TableIdentifier& table_identifier);

        void
        drop_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name,
            txn::Transaction& txn);

        std::vector<types::MetaTable*>
        get_all_tables() const;

        std::vector<types::MetaSchema*>
        get_all_schemas() const;
    };
}

#endif //DELTABASE_DDL_SERVICE_HPP