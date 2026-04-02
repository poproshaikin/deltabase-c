//
// Created by poproshaikin on 28.11.25.
//

#ifndef DELTABASE_STD_DB_INSTANCE_HPP
#define DELTABASE_STD_DB_INSTANCE_HPP

#include "../../recovery/include/recovery_manager.hpp"
#include "../../transactions/include/transaction_manager.hpp"
#include "../../types/include/config.hpp"
#include "../../wal/include/wal_manager.hpp"
#include "buffer_pool.hpp"
#include "catalog.hpp"
#include "db_instance.hpp"
#include "io_manager.hpp"

namespace storage
{
    class StdDbInstance final : public IDbInstance
    {
        types::Config cfg_;
        std::unique_ptr<IIOManager> io_manager_;
        std::unique_ptr<wal::IWALManager> wal_manager_;
        std::unique_ptr<txn::TransactionManager> txn_manager_;
        std::unique_ptr<recovery::RecoveryManager> recovery_manager_;
        std::unique_ptr<BufferPool> buffer_pool_;
        std::unique_ptr<CatalogCache> catalog_;

        void
        init();

        ssize_t
        has_available_page(const std::vector<const types::DataPage*>& vec, size_t size) const;

        bool
        is_row_obsolete(const types::RowPtr& row_ptr) const;

    public:
        explicit StdDbInstance(const types::Config& cfg);

        ~StdDbInstance() override;

        bool
        needs_stream(types::IPlanNode& plan_node) override;

        types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) override;

        txn::Transaction
        make_txn() override;

        void
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            std::vector<types::DataToken> row,
            txn::Transaction& txn
        ) override;

        void
        insert_row_into_indexes(
            const types::MetaTable& mt, const types::DataRow& row, const types::DataPageId& page_id
        ) override;

        void
        update_row(
            const std::string& table_name,
            const std::string& schema_name,
            types::RowUpdate update,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn
        ) override;

        void
        delete_rows(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::DataRow>& rows,
            txn::Transaction& txn
        ) override;

        types::MetaTable*
        get_table(const std::string& table_name, const std::string& schema_name) override;

        types::MetaTable*
        get_table(const types::TableIdentifier& identifier) override;

        const types::Config&
        get_config() const override;

        types::MetaSchema*
        get_schema(const std::string& name) override;

        bool
        exists_table(const std::string& table_name, const std::string& schema_name) override;

        bool
        exists_table(const types::TableIdentifier& identifier) override;

        bool
        exists_db(const std::string& name) override;

        void
        create_table(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::ColumnDefinition>& columns,
            txn::Transaction& txn
        ) override;

        void
        create_schema(const std::string& schema_name, txn::Transaction& txn) override;

        bool
        exists_schema(const std::string& schema_name) override;

        void
        create_index(
            const std::string& string,
            const std::string& table_name,
            const std::string& column_name,
            const std::string& schema_name,
            bool is_unique,
            txn::Transaction& txn
        ) override;

        bool
        exists_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name
        ) override;

        bool
        exists_index(
            const std::string& index_name, const types::TableIdentifier& table_identifier
        ) override;

        types::MetaIndex*
        get_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name
        ) override;

        types::MetaIndex*
        get_index(
            const std::string& index_name, const types::TableIdentifier& table_identifier
        ) override;

        void
        drop_index(
            const std::string& index_name,
            const std::string& table_name,
            const std::string& schema_name,
            txn::Transaction& txn
        ) override;
    };
} // namespace storage

#endif