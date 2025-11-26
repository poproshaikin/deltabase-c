//
// Created by poproshaikin on 25.11.25.
//

#include "include/catalog_snapshot.hpp"

#include <ranges>

namespace types
{
    CatalogSnapshot::CatalogSnapshot(
        const std::unordered_map<Uuid, MetaTable>& tables,
        const std::unordered_map<Uuid, MetaSchema>& schemas
    ) : version(last_version_++)
    {
        this->tables.reserve(tables.size());
        for (const auto& entry : tables)
        {
            this->tables.emplace(std::make_pair(entry.first, entry.second));
        }

        this->schemas.reserve(schemas.size());
        for (const auto& entry : schemas)
        {
            this->schemas.emplace(std::make_pair(entry.first, entry.second));
        }
    }

    MetaSchema
    CatalogSnapshot::get_schema(const std::string& schema_name)
    {
        for (const auto& schema : schemas | std::views::values)
            if (schema.value.name == schema_name)
                return schema.value;

        throw std::logic_error("CatalogSnapshot::get_schema: schema " + schema_name + " not found");
    }

    MetaTable
    CatalogSnapshot::get_table(const std::string& table_name, const std::string& schema_name)
    {
        auto schema = get_schema(schema_name);

        for (const auto& table : tables | std::views::values)
            if (table.value.name == table_name && table.value.schema_id == schema.id)
                return table.value;

        throw std::logic_error("CatalogSnapshot::get_table: table " + table_name + " not found");
    }

    bool
    CatalogSnapshot::operator==(const CatalogSnapshot& other) const
    {
        return this->version == other.version;
    }

    bool
    CatalogSnapshot::operator!=(const CatalogSnapshot& other) const
    {
        return !(*this == other);
    }

}