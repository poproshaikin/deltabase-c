#include "include/storage.hpp"

#include "../misc/include/exceptions.hpp"
#include "include/objects/meta_object.hpp"
#include "include/cache/accessors.hpp"
#include "include/cache/key_extractor.hpp"
#include "include/paths.hpp"
#include "wal/wal_manager.hpp"
#include <filesystem>
#include <ranges>

namespace storage
{
    void
    Storage::ensure_attached(const std::string& method_name) const
    {
        if (!db_name_ || !schemas_ || !tables_)
            throw std::runtime_error(
                "Storage::" + method_name + ": storage is not attached to a db"
            );
    }

    Storage::Storage(const fs::path& data_dir) : data_dir_(data_dir), fm_(data_dir)
    {
    }

    Storage::Storage(const fs::path& data_dir, const std::string& db_name)
        : fm_(data_dir), data_dir_(data_dir)
    {
        load(db_name);
        attach_db(db_name);
    }

    void
    Storage::load(const std::string& db_name)
    {
        auto tables = fm_.load_all_tables(db_name);
        auto schemas = fm_.load_all_schemas(db_name);

        tables_->init_with(std::move(tables));
        schemas_->init_with(std::move(schemas));
    }

    void
    Storage::attach_db(const std::string& db_name)
    {
        db_name_ = db_name;
        schemas_.emplace();
        tables_.emplace();
        wal_.emplace(db_name, fm_);
        page_buffers_.emplace(db_name, fm_, schemas_.value(), tables_.value());
        checkpoint_ctl_.emplace(*wal_, *page_buffers_);
    }

    void
    Storage::create_database(const std::string& db_name) const
    {
        const auto path = path_db(data_dir_, db_name);
        fs::create_directory(path);
    }

    void
    Storage::drop_database(const std::string& db_name) const
    {
        auto path = path_db(data_dir_, db_name);
        if (fs::exists(path))
            fs::remove_all(path);
    }

    bool
    Storage::exists_database(const std::string& db_name) const
    {
        auto path = path_db(data_dir_, db_name);
        return fs::exists(path);
    }

    // ----- Schemas -----

    void
    Storage::create_schema(MetaSchema&& schema)
    {
        ensure_attached("create_schema");

        wal_->push_create_schema(schema);
        schemas_->put(std::move(schema));
    }

    MetaSchema&
    Storage::get_schema(const std::string& schema_name)
    {
        ensure_attached("get_schema");

        std::string schema_key = make_schema_key(schema_name);
        if (!schemas_->has(schema_key))
            throw std::runtime_error("Storage::get_schema: schema " + schema_key + " doesnt exist");

        return schemas_->get(schema_key);
    }

    MetaSchema&
    Storage::get_schema(const sql::TableIdentifier& identifier)
    {
        return get_schema(identifier.schema_name.value().value);
    }

    bool
    Storage::exists_schema(const std::string& schema_name) const
    {
        ensure_attached("exists_schema");

        std::string schema_key = make_schema_key(schema_name);
        return schemas_->has(schema_key);
    }

    std::optional<std::string>
    Storage::find_schema_key(const std::string& schema_id) const
    {
        ensure_attached("find_schema_name");

        for (const auto& entry : *schemas_)
            if (entry.second.id == schema_id)
                return make_key(entry.second);

        return std::nullopt;
    }

    bool
    Storage::exists_schema_by_id(const std::string& schema_id) const
    {
        ensure_attached("exists_schema_by_id");

        return schemas_->has(schema_id);
    }

    MetaSchema&
    Storage::get_schema_by_id(const std::string& id)
    {
        ensure_attached("get_schema_by_id");

        return schemas_->get(id);
    }

    void
    Storage::drop_schema(const std::string& schema_name)
    {
        ensure_attached("drop_schema");

        const MetaSchema* schema = schemas_->find_first([schema_name](const MetaSchema& val)
        {
            return val.name == schema_name;
        });
        if (!schema)
            throw std::runtime_error(
                "Storage::drop_schema: schema " + schema_name + " doesnt exist"
            );

        schemas_->remove(*schema);
        wal_->push_drop_schema(*schema);

        auto dir_path = path_db_schema(data_dir_, db_name_.value(), schema->name);
        fs::remove_all(dir_path);
    }

    // ----- Tables -----

    void
    Storage::create_table(MetaTable&& table)
    {
        ensure_attached("create_table");

        if (!exists_schema_by_id(table.schema_id))
            throw std::runtime_error(
                "Storage::create_table: schema with id " + table.schema_id + " does not exist"
            );

        wal_->push_create_table(table);
        tables_->put(std::move(table));
    }

    bool
    Storage::exists_table(const std::string& schema_name, const std::string& name)
    {
        ensure_attached("exists_table");

        const std::string* schema_id = nullptr;
        for (const auto& val : *schemas_ | std::views::values)
            if (val.name == schema_name)
                schema_id = &val.id;

        if (!schema_id)
            return false;

        const std::string* table_id = nullptr;
        for (const auto& val : *tables_ | std::views::values)
            if (val.name == name)
                table_id = &val.name;

        return table_id;
    }

    bool
    Storage::exists_table(const sql::TableIdentifier& identifier)
    {
        return exists_table(identifier.schema_name->value, identifier.table_name.value);
    }

    MetaTable&
    Storage::get_table(const std::string& name)
    {
        ensure_attached("get_table");

        auto* ptr = tables_->find_first([name](const MetaTable& table)
        {
            return table.name == name;
        });

        if (!ptr)
            throw TableDoesntExist(name);

        return *ptr;
    }

    MetaTable&
    Storage::get_table(const std::string& table_name, const std::string& schema_name)
    {
        const auto* schema = schemas_->find_first([schema_name](const MetaSchema& val)
        {
            return val.name == schema_name;
        });

        if (!schema)
            throw SchemaDoesntExist(schema_name);

        auto* table = tables_->find_first([table_name, schema](const MetaTable& val)
        {
            return val.name == table_name && val.schema_id == schema->id;
        });

        if (!table)
            throw TableDoesntExist(table_name);

        return *table;
    }

    MetaTable&
    Storage::get_table(const sql::TableIdentifier& identifier)
    {
        return get_table(identifier.table_name.value, identifier.schema_name->value);
    }

    std::optional<std::string>
    Storage::find_table_key(const std::string& table_id) const
    {
        ensure_attached("find_table_key");

        for (const auto& val : *tables_ | std::views::values)
        {
            if (val.id == table_id)
            {
                return make_key(val);
            }
        }

        return std::nullopt;
    }

    bool
    Storage::exists_table_by_id(const std::string& table_id) const
    {
        ensure_attached("exists_table_by_id");
        return find_table_key(table_id) != std::nullopt;
    }

    MetaTable&
    Storage::get_table_by_id(const std::string& id)
    {
        ensure_attached("get_table_by_id");
        return tables_->get(id);
    }

    void
    Storage::update_table(const MetaTable& new_table)
    {
    }

    // ----- Data -----

    void
    Storage::insert_row(MetaTable& table, DataRow row)
    {
        if (!page_buffers_.has_value())
            throw std::runtime_error("Storage::insert_row: storage is not attached to a db");

        wal_->push_insert(table, row);
        page_buffers_->insert_row(table, row);
    }

    uint64_t
    Storage::update_rows(
        MetaTable& table,
        const DataFilter& filter,
        const DataRowUpdate& update
    )
    {
        return page_buffers_->update_rows(table, filter, update);
    }

    uint64_t
    Storage::delete_rows(MetaTable& table, const std::optional<DataFilter>& filter)
    {
    }

    DataTable
    seq_scan(
        const MetaTable& table,
        const std::vector<std::string>& column_names,
        const std::optional<DataFilter>& filter
    )
    {
    }
} // namespace storage