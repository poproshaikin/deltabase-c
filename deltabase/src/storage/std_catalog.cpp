//
// Created by poproshaikin on 25.11.25.
//

#include "include/std_catalog.hpp"

namespace storage
{
    using namespace types;

    StdCatalog::StdCatalog(IIOManager& io_manager) : io_manager_(io_manager), actual_snapshot_()
    {
        init();
    }

    void
    StdCatalog::init()
    {
        auto tables = io_manager_.load_tables();
        auto schemas = io_manager_.load_schemas();

        std::unordered_map<Uuid, MetaTable> tables_map;
        tables_map.reserve(tables.size());

        std::unordered_map<Uuid, MetaSchema> schemas_map;
        schemas_map.reserve(schemas.size());

        for (const auto& table : tables)
            tables_map.emplace(table.id, std::move(table));

        for (const auto& schema : schemas)
            schemas_map.emplace(schema.id, std::move(schema));

        actual_snapshot_ = CatalogSnapshot(tables_map, schemas_map);
        stored_snapshot_ = actual_snapshot_;
    }

    CatalogSnapshot
    StdCatalog::get_snapshot()
    {
        return actual_snapshot_;
    }

    void
    StdCatalog::save_snapshot(const CatalogSnapshot& snapshot)
    {
        if (snapshot == actual_snapshot_)
            return;

        snapshots_.push_back(std::move(actual_snapshot_));
        actual_snapshot_ = snapshot;
    }

    void
    StdCatalog::flush()
    {
        if (stored_snapshot_ == actual_snapshot_)
            return;
    }

}