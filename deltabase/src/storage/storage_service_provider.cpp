//
// Created by poproshaikin on 6/11/26.
//

#include "storage_service_provider.hpp"

#include "io_manager_factory.hpp"
#include "wal_manager_factory.hpp"

namespace storage
{

    StorageServiceProvider::StorageServiceProvider(const types::Config& cfg)
    {
        if (!std::filesystem::exists(cfg.db_path))
            std::filesystem::create_directories(cfg.db_path);

        IOManagerFactory io_factory;
        io_manager_ = io_factory.make(cfg);
        wal::WalManagerFactory wal_factory;
        wal_manager_ = wal_factory.make(cfg);
        buffer_pool_ = std::make_unique<BufferPool>(*io_manager_);
        catalog_ = std::make_unique<CatalogCache>(*io_manager_);
    }

    DDLService
    StorageServiceProvider::create_ddl()
    {
        return DDLService(*catalog_, *wal_manager_);
    }
}
