//
// Created by poproshaikin on 07.03.26.
//

#ifndef DELTABASE_WAL_MANAGER_FACTORY_HPP
#define DELTABASE_WAL_MANAGER_FACTORY_HPP
#include "config.hpp"
#include "db_io_lock_service.hpp"
#include "file_wal_manager.hpp"
#include "wal_manager.hpp"

namespace wal
{
    class WalManagerFactory
    {
    public:
        std::unique_ptr<IWALManager>
        make(const types::Config& cfg) const
        {
            return make(cfg, storage::DatabaseIoLockService::shared());
        }

        std::unique_ptr<IWALManager>
        make(const types::Config& cfg, std::shared_ptr<storage::DatabaseIoLockService> io_lock_service) const
        {
            switch (cfg.io_type)
            {
            case types::Config::IoType::File:
                return std::make_unique<FileWalManager>(
                    cfg.db_path, cfg.db_name.value(), cfg.serializer_type, std::move(io_lock_service)
                );
            default:
                throw std::runtime_error("WalManagerFactory::make(): invalid io type " + std::to_string(static_cast<int>(cfg.io_type)));
            }
        }
    };
} // namespace wal

#endif // DELTABASE_WAL_MANAGER_FACTORY_HPP
