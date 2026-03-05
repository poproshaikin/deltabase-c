//
// Created by poproshaikin on 08.01.26.
//

#ifndef DELTABASE_IO_MANAGER_FACTORY_HPP
#define DELTABASE_IO_MANAGER_FACTORY_HPP
#include "detached_file_io_manager.hpp"
#include "file_io_manager.hpp"
#include "io_manager.hpp"

namespace storage
{
    class IOManagerFactory
    {
    public:
        std::unique_ptr<IIOManager>
        make(const types::Config& cfg) const
        {
            switch (cfg.io_type)
            {
            case types::Config::IoType::File:
                return std::make_unique<FileIOManager>(
                    cfg.db_path, cfg.db_name.value(), cfg.serializer_type
                );
            case types::Config::IoType::DetachedFile:
                return std::make_unique<DetachedFileIOManager>(cfg.db_path);
            default:
                throw std::runtime_error(
                    "StdDbInstance::StdDbInstance: unknown IO type " +
                    std::to_string(static_cast<int>(cfg.io_type))
                );
            }
        }
    };
} // namespace storage

#endif // DELTABASE_IO_MANAGER_FACTORY_HPP