//
// Created by poproshaikin on 26.11.25.
//

#include "std_storage.hpp"

#include "file_io_manager.hpp"
#include "std_binary_serializer.hpp"
#include "std_data_buffers.hpp"
#include "std_catalog.hpp"

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

    CatalogSnapshot
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

    }


}