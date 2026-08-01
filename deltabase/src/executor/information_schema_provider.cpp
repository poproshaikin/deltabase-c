#include "include/information_schema_provider.hpp"
#include "../storage/include/storage_service_provider.hpp"

#include "meta_column.hpp"
#include "meta_table.hpp"
#include "UUID.hpp"

#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <string>

namespace exq
{
    using namespace types;

    namespace
    {
        DataToken
        make_text(const std::string& s)
        {
            Bytes bytes(s.begin(), s.end());
            return DataToken(bytes, DataType::TEXT);
        }

        DataToken
        make_int(int v)
        {
            Bytes bytes(sizeof(int));
            std::memcpy(bytes.data(), &v, sizeof(int));
            return DataToken(bytes, DataType::INTEGER);
        }

        DataToken
        make_null()
        {
            return DataToken(Bytes{}, DataType::_NULL);
        }

        MetaTable
        make_tables_meta()
        {
            MetaTable mt;
            mt.name = "tables";
            mt.columns = {
                MetaColumn("table_catalog", DataType::TEXT,    {}),
                MetaColumn("table_schema",  DataType::TEXT,    {}),
                MetaColumn("table_name",    DataType::TEXT,    {}),
                MetaColumn("table_type",    DataType::TEXT,    {}),
                MetaColumn("primary_key",   DataType::TEXT,    {}),
                MetaColumn("column_count",  DataType::INTEGER, {}),
                MetaColumn("index_count",   DataType::INTEGER, {}),
                MetaColumn("total_rows",    DataType::INTEGER, {}),
                MetaColumn("live_rows",     DataType::INTEGER, {}),
            };
            return mt;
        }
    } // namespace

    InformationSchemaProvider::InformationSchemaProvider(storage::StorageServiceProvider& ssp)
        : ssp_(ssp)
    {
    }

    bool
    InformationSchemaProvider::is_virtual(const TableIdentifier& table) const
    {
        return is_virtual(
            table.schema_name.has_value()
                ? std::optional(table.schema_name.value().value)
                : std::nullopt,
            table.table_name.value);
    }

    bool
    InformationSchemaProvider::is_virtual(const std::optional<std::string>& schema_name,
                                          const std::string& table_name) const
    {
        if (schema_name.has_value() && schema_name.value() == "information_schema")
            return table_name == "tables";

        return false;
    }

    MetaTable
    InformationSchemaProvider::get_virtual_table(const TableIdentifier& table) const
    {
        const auto& config = ssp_.config();
        return get_virtual_table(
            table.schema_name.has_value() ? table.schema_name.value().value : config.default_schema,
            table.table_name.value);
    }

    MetaTable
    InformationSchemaProvider::get_virtual_table(
        const std::string& schema_name,
        const std::string& table_name) const
    {
        if (!is_virtual(schema_name, table_name))
            throw std::invalid_argument("Not a virtual table: " + schema_name + "." + table_name);

        return make_tables_meta();
    }

    DataTable
    InformationSchemaProvider::get_virtual_data(const TableIdentifier& table) const
    {
        const auto& config = ssp_.config();
        auto schema_name = table.schema_name.has_value()
            ? table.schema_name.value().value
            : config.default_schema;

        return get_virtual_data(schema_name, table.table_name.value);
    }

    DataTable
    InformationSchemaProvider::get_virtual_data(
        const std::string& schema_name,
        const std::string& table_name) const
    {
        if (!is_virtual(schema_name, table_name))
            return DataTable{};

        const auto& config = ssp_.config();

        auto all_schemas = ssp_.ddl().get_all_schemas();
        std::unordered_map<UUID, std::string> schema_names;
        schema_names.reserve(all_schemas.size());
        for (const auto* s : all_schemas)
            schema_names.emplace(s->id, s->name);

        DataTable result;
        result.table_name = table_name;
        result.output_schema = {
            {"table_catalog", DataType::TEXT},
            {"table_schema",  DataType::TEXT},
            {"table_name",    DataType::TEXT},
            {"table_type",    DataType::TEXT},
            {"primary_key",   DataType::TEXT},
            {"column_count",  DataType::INTEGER},
            {"index_count",   DataType::INTEGER},
            {"total_rows",    DataType::INTEGER},
            {"live_rows",     DataType::INTEGER},
        };

        const std::string db_name = config.db_name.value_or("");

        for (const auto* mt : ssp_.ddl().get_all_tables())
        {
            auto it = schema_names.find(mt->schema_id);
            const std::string mt_schema = it != schema_names.end() ? it->second : "";

            const MetaColumn* pkey = nullptr;
            for (const auto& col : mt->columns)
                if (col.has_constraint<MetaPrimaryKeyConstraint>())
                {
                    pkey = &col;
                    break;
                }

            DataRow row;
            row.tokens = {
                db_name.empty() ? make_null() : make_text(db_name),
                make_text(mt_schema),
                make_text(mt->name),
                make_text("BASE TABLE"),
                make_text(pkey ? pkey->name : "NULL"),
                make_int(static_cast<int>(mt->columns.size())),
                make_int(static_cast<int>(mt->indexes.size())),
                make_int(static_cast<int>(mt->total_rows)),
                make_int(static_cast<int>(mt->live_rows)),
            };
            result.rows.push_back(std::move(row));
        }

        return result;
    }
} // namespace exq
