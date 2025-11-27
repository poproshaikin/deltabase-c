//
// Created by poproshaikin on 24.11.25.
//

#ifndef DELTABASE_STD_STORAGE_HPP
#define DELTABASE_STD_STORAGE_HPP
#include "catalog.hpp"
#include "data_buffers.hpp"
#include "io_manager.hpp"
#include "storage.hpp"
#include "wal_manager.hpp"
#include "../../types/include/db_cfg.hpp"

namespace storage
{
    class StdStorage final : public IStorage
    {
        types::DbConfig cfg_;
        std::unique_ptr<IIOManager> io_manager_;
        std::unique_ptr<ICatalog> catalog_;
        std::unique_ptr<IDataBuffers> buffers_;
        std::unique_ptr<IWalManager> wal_manager_;

    public:
        explicit
        StdStorage(const types::DbConfig& cfg);

        const types::CatalogSnapshot
        get_catalog_snapshot() override;

        bool
        needs_stream(types::IPlanNode& plan_node) override;

        types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) override;

        types::Transaction
        begin_txn() override;

        uint64_t
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            types::DataRow& row,
            types::Transaction txn
        ) override;

        void
        commit_txn(types::Transaction txn) override;
    };
}

#endif //DELTABASE_STD_STORAGE_HPP