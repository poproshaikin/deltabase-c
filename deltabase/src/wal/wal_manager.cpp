//
// Created by poproshaikin on 05.03.26.
//

#include "include/wal_manager.hpp"

#include "include/wal_io_manager_factory.hpp"
namespace wal
{
    WalManager::WalManager(const types::Config& cfg)
    {
        WalIOManagerFactory wal_io_factory;
        wal_io_manager = wal_io_factory.make(cfg);
    }
} // namespace wal
