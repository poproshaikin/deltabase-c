//
// Created by poproshaikin on 08.01.26.
//

#include "io_manager_factory.hpp"

#include "detached_file_io_manager.hpp"
#include "file_io_manager.hpp"

namespace storage
{
    using namespace types;

    std::unique_ptr<IIOManager>
    IOManagerFactory::make_io_manager(const Config& cfg) const
    {
        switch (cfg.io_type)
        {
        case Config::IoType::File:
            return std::make_unique<FileIOManager>(
                cfg.db_path, cfg.db_name.value(), cfg.serializer_type
            );
        case Config::IoType::DetachedFile:
            return std::make_unique<DetachedFileIOManager>(cfg.db_path);
        default:
            throw std::runtime_error(
                "StdDbInstance::StdDbInstance: unknown IO type " +
                std::to_string(static_cast<int>(cfg.io_type))
            );
        }
    }
} // namespace storage