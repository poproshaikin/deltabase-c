#include "include/storage.hpp"
#include "include/cache/accessors.hpp"
#include "include/cache/key_extractor.hpp"
#include "include/objects/meta_object.hpp"
#include "include/paths.hpp"
#include "wal/wal_manager.hpp"
#include <filesystem>

namespace storage
{
    Storage::Storage(const fs::path& data_dir) : data_dir_(data_dir), fm_(data_dir)
    {
    }

    Storage::Storage(const fs::path& data_dir, const std::string& db_name)
        : data_dir_(data_dir), fm_(data_dir)
    {
        attach_db(db_name);
    }

    void
    Storage::ensure_attached(const std::string& method_name) const {
        if (!db_name_ || !schemas_ || !tables_)
            throw std::runtime_error("Storage::" + method_name + ": storage is not attached to a db");
    }

    void
    Storage::attach_db(const std::string& db_name) {
        db_name_ = db_name;
        schemas_.emplace(MetaSchemaAccessor(db_name, fm_));
        tables_.emplace(MetaTableAccessor(db_name, fm_));
        wal_.emplace(db_name, fm_);
        page_buffers_.emplace(db_name, fm_, schemas_.value(), tables_.value());
        checkpoint_ctl_.emplace(*wal_, *page_buffers_);
    }

    void
    Storage::create_database(const std::string& db_name) {
        auto path = path_db(data_dir_, db_name);
        fs::create_directory(path);
    }

    void
    Storage::drop_database(const std::string& db_name) {
        auto path = path_db(data_dir_, db_name);
        if (fs::exists(path)) 
            fs::remove_all(path);
    }

    bool
    Storage::exists_database(const std::string& db_name) {
        auto path = path_db(data_dir_, db_name);
        return fs::exists(path);
    }

    // ----- Schemas -----

    void
    Storage::create_schema(MetaSchema&& schema) {
        ensure_attached("create_schema");

        // auto dir_path = path_db_schema(data_dir_, db_name_.value(), schema.name);

        // if (fs::exists(dir_path)) 
        //     throw std::runtime_error("Storage::create_schema: schema " + schema.name + " already exists");

        // if (!fs::create_directory(dir_path)) 
        //     throw std::runtime_error("Storage::create_schema: failed to create directory " + std::string(dir_path));

        wal_->push_create_schema(schema);
        schemas_->put(schema);

        // auto mf_path = path_db_schema_meta(data_dir_, db_name_.value(), schema.name); 
        // std::ofstream metafile(mf_path, std::ios::binary);
        // if (!metafile) 
        //     throw std::runtime_error("Storage::create_schema: failed to open metafile at path " + std::string(mf_path));

        // bytes_v serialized = schema.serialize();
        // metafile.write(reinterpret_cast<const char*>(serialized.data()), serialized.size());
        // metafile.close();
    }

    MetaSchema&
    Storage::get_schema(const std::string& schema_name) const { 
        ensure_attached("get_schema");

        std::string schema_key = make_schema_key(schema_name);
        if (!schemas_->has(schema_key)) 
            throw std::runtime_error("Storage::get_schema: schema " + schema_key + " doesnt exist");
        
        return schemas_->get(schema_key);
    }

    MetaSchema&
    Storage::get_schema(const sql::TableIdentifier& identifier) const { 
        return get_schema(identifier.schema_name.value().value);
    }

    bool
    Storage::exists_schema(const std::string& schema_name) const {
        ensure_attached("exists_schema");

        std::string schema_key = make_schema_key(schema_name);
        return schemas_->has(schema_key);
    }

    std::optional<std::string>
    Storage::find_schema_key(const std::string& schema_id) const {
        ensure_attached("find_schema_name");

        for (const auto& entry : *schemas_) {
            if (entry.value.id == schema_id) {
                return make_key(entry.value);
            }
        }

        return std::nullopt;
    }

    bool
    Storage::exists_schema_by_id(const std::string& schema_id) const {
        ensure_attached("exists_schema_by_id");

        return find_schema_key(schema_id) != std::nullopt;
    }

    MetaSchema&
    Storage::get_schema_by_id(const std::string& id) const {
        ensure_attached("get_schema_by_id");

        auto name = find_schema_key(id);
        if (name == std::nullopt) 
            throw std::runtime_error("Storage::get_schema_by_id: schema with id " + id + " doesnt exist");
        
        return get_schema(*name);
    }

    void
    Storage::drop_schema(const std::string& schema_name) {
        ensure_attached("drop_schema");

        std::string schema_key = make_schema_key(schema_name);
        if (!schemas_->has(schema_key)) 
            throw std::runtime_error("Storage::drop_schema: schema " + schema_name + " doesnt exist");
        
        MetaSchema schema = schemas_->remove(schema_key);
        wal_->push_drop_schema(schema);

        auto dir_path = path_db_schema(data_dir_, db_name_.value(), schema.name);
        fs::remove_all(dir_path);
    }

    // ----- Tables -----

    void
    Storage::create_table(MetaTable&& table) {
        ensure_attached("create_table");

        if (!exists_schema_by_id(table.schema_id)) 
            throw std::runtime_error("Storage::create_table: schema with id " + table.schema_id + " does not exist");

        // const auto& schema = get_schema_by_id(table.schema_id);
        // auto path = path_db_schema_table(data_dir_, *db_name_, schema.name, table.name);

        // if (fs::exists(path) && tables_->has(table))   
        //     throw std::runtime_error("Storage::create_table: table " + table.name + " already exists");

        // try {
        //     fs::create_directory(path);
        // } catch (const fs::filesystem_error& err) {
        //     throw std::runtime_error("Storage::create_table: fs_error in fs::create_directory: " + std::string(err.what()));
        // } 

        wal_->push_create_table(table);
        tables_->put(table);

        // auto mf_path = path_db_schema_table_meta(data_dir_, *db_name_, schema.name, table.name);
        // bytes_v serialized = table.serialize();

        // std::ofstream metafile(mf_path, std::ios::binary);
        // if (!metafile) 
        //     throw std::runtime_error("Storage::create_schema: failed to open metafile at path " + std::string(mf_path));

        // metafile.write(reinterpret_cast<const char*>(serialized.data()), serialized.size());
        // metafile.close();
    }

    bool
    Storage::exists_table(const std::string& schema_name, const std::string& name) {
        ensure_attached("exists_table");

        const std::string* schema_id = nullptr;
        for (const auto& schema : *schemas_)
            if (schema.value.name == schema_name)
                schema_id = &schema.value.id;

        if (!schema_id)
            return false;

        const std::string * table_id = nullptr;
        for (const auto& table : *tables_)
            if (table.value.name == name)
                table_id = &table.value.name;

        return table_id;
    }

    bool
    Storage::exists_table(const sql::TableIdentifier& identifier) {
        return exists_table(identifier.schema_name->value, identifier.table_name.value);
    }

    MetaTable&
    Storage::get_table(const std::string& name) {
        ensure_attached("get_table");

        return tables_->get(name);
    }

    std::optional<std::string>
    Storage::find_table_key(const std::string& table_id) const {
        ensure_attached("find_table_key");

        for (const auto& entry : *tables_) {
            if (entry.value.id == table_id) {
                return make_key(entry.value);
            }
        }

        return std::nullopt;
    }

    bool
    Storage::exists_table_by_id(const std::string& table_id) {
        ensure_attached("exists_table_by_id");
        return find_table_key(table_id) != std::nullopt;
    }

    MetaTable&
    Storage::get_table_by_id(const std::string& id) {
        ensure_attached("get_table_by_id");
        auto key = find_table_key(id);
        if (key == std::nullopt) 
            throw std::runtime_error("Storage::get_table_by_id: table with id " + id + " doesnt exist");
        return tables_->get(*key);
    }

    void
    Storage::update_table(const MetaTable& new_table) {

    }

    // ----- Data -----

    void
    Storage::insert_row(MetaTable& table, DataRow row) {
        if (!page_buffers_.has_value()) 
            throw std::runtime_error("Storage::insert_row: storage is not attached to a db");

        wal_->push_insert(table, row);
        page_buffers_->insert_row(table, row);
    }

    uint64_t
    update_rows_by_filter(MetaTable& table, const DataFilter& filter, const DataRowUpdate& update);

    uint64_t
    delete_rows_by_filter(MetaTable& table, const std::optional<DataFilter>& filter);

    DataTable
    seq_scan(
        const MetaTable& table,
        const std::vector<std::string>& column_names,
        const std::optional<DataFilter>& filter
    );
} // namespace storage