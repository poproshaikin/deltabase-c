//
// Created by poproshaikin on 26.11.25.
//

#include "std_storage.hpp"

#include "file_io_manager.hpp"
#include "std_binary_serializer.hpp"
#include "std_data_buffers.hpp"
#include "std_catalog.hpp"

#include "../../misc/include/convert.hpp"

namespace storage
{
    using namespace types;

    StdStorage::StdStorage(const DbConfig& cfg) : cfg_(cfg)
    {
        switch (cfg.io_system_type)
        {
        case DbConfig::IOSysType::File:
            io_manager_ = std::make_unique<FileIOManager>(std::make_unique<StdBinarySerializer>());
            break;
        default:
            throw std::runtime_error("StdStorage::StdStorage: Unknown IOSysType type");
        }

        buffers_ = std::make_unique<StdDataBuffers>(*io_manager_);
        catalog_ = std::make_unique<StdCatalog>(*io_manager_);
    }

    const CatalogSnapshot
    StdStorage::get_catalog_snapshot()
    {
        return catalog_->get_snapshot();
    }

    bool
    StdStorage::needs_stream(IPlanNode& plan_node)
    {
        // TODO
        return false;
    }

    DataTable
    StdStorage::seq_scan(const std::string& table_name, const std::string& schema_name)
    {
        auto catalog = get_catalog_snapshot();
        auto mt = catalog.get_table(table_name, schema_name);
        auto pages = buffers_->get_pages(mt);

        DataTable dt;
        dt.output_schema = misc::convert(mt);

        uint64_t rows_count = 0;
        for (const auto& page : pages)
            rows_count += page.rows.size();

        dt.rows.reserve(rows_count);

        for (const auto& page : pages)
            for (const auto& row : page.rows)
                dt.rows.push_back(row);

        return dt;
    }

    Transaction
    StdStorage::begin_txn()
    {
        Transaction tx;
        wal_manager_->push_log(BeginTransactionRecord{.txn_id = tx.id});
        return tx;
    }

    uint64_t
    StdStorage::insert_row(
        const std::string& table_name,
        const std::string& schema_name,
        DataRow& row,
        Transaction txn
    )
    {
        auto snapshot = catalog_->get_snapshot();
        auto table = snapshot.get_table(table_name, schema_name);
        wal_manager_->push_log(InsertRecord{.table_id = table.id, .row = row, .txn_id = txn.id});
        buffers_->insert_row(table, row);
        snapshot.version += CatalogSnapshot::last_version_++;
        catalog_->commit_snapshot(snapshot);
    }
}