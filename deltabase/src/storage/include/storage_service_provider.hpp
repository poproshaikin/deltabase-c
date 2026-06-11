//
// Created by poproshaikin on 6/11/26.
//

#ifndef DELTABASE_STORAGE_SERVICE_PROVIDER_HPP
#define DELTABASE_STORAGE_SERVICE_PROVIDER_HPP
#include "buffer_pool.hpp"
#include "catalog.hpp"
#include "ddl_service.hpp"
#include "wal_manager.hpp"

namespace storage
{
    class StorageServiceProvider
    {
        std::unique_ptr<BufferPool> buffer_pool_;
        std::unique_ptr<CatalogCache> catalog_;
        std::unique_ptr<IIOManager> io_manager_;
        std::unique_ptr<wal::IWALManager> wal_manager_;

    public:
        explicit StorageServiceProvider(const types::Config& cfg);

        DDLService
        create_ddl();
    };
}

#endif //DELTABASE_STORAGE_SERVICE_PROVIDER_HPP
