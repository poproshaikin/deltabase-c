//
// Created by poproshaikin on 05.03.26.
//

#ifndef DELTABASE_WALIOMANAGERFACTORY_HPP
#define DELTABASE_WALIOMANAGERFACTORY_HPP
#include "config.hpp"
#include "detached_file_wal_io_manager.hpp"
#include "file_wal_io_manager.hpp"
#include "wal_io_manager.hpp"

namespace wal
{
    class WalIOManagerFactory
    {
    public:
        std::unique_ptr<IWalIOManager>
        make(const types::Config& cfg) const
        {
            switch (cfg.io_type)
            {
            case types::Config::IoType::File:
                return std::make_unique<FileWalIOManager>(
                    cfg.db_path, cfg.db_name.value(), cfg.serializer_type
                );
            case types::Config::IoType::DetachedFile:
                return std::make_unique<DetachedFileWalIOManager>();
            default:
                throw std::runtime_error(
                    "WalIOManagerFactory: unknown io type " +
                    std::to_string(static_cast<int>(cfg.io_type))
                );
            }
        }
    };
} // namespace wal

#endif // DELTABASE_WALIOMANAGERFACTORY_HPP
