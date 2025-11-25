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
    ) : version(last_version_++), tables(tables), schemas(schemas)
    {
    }

    MetaSchema
    CatalogSnapshot::get_schema(const std::string& schema_name)
    {
        for (const auto& schema : schemas | std::views::values)
            if (schema.name == schema_name)
                return schema;

        throw std::logic_error("CatalogSnapshot::get_schema: schema " + schema_name + " not found");
    }

    MetaTable
    CatalogSnapshot::get_table(const std::string& table_name, const std::string& schema_name)
    {
        auto schema = get_schema(table_name);

        for (const auto& table : tables | std::views::values)
            if (table.name == table_name && table.schema_id == schema.id)
                return table;

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