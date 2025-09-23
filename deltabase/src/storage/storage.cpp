#include "include/storage.hpp"
#include "include/cache/accessors.hpp"
#include "include/objects/meta_object.hpp"

namespace storage {
    Storage::Storage() : schemas_(MetaSchemaAccessor()) {

    }

    Storage(fs::path data_dir) {

    }

    void
    Storage::create_database(const std::string& db_name) {
        auto path = path_db();
    }

    void
    Storage::drop_database(const std::string& db_name) {

    }

    bool
    exists_database(const std::string& db_name);

    std::vector<std::string>
    get_databases();

    // ----- Schemas -----

    void
    create_schema(const MetaSchema& schema);

    const MetaSchema&
    get_schema(const std::string& schema_name);
    const MetaSchema&
    get_schema(const std::string& schema_name, const std::string& db_name);
    const MetaSchema&
    get_schema_by_id(const std::string& id);

    int
    drop_schema(const std::string& schema_name);
    int
    drop_schema(const std::string& schema_name, const std::string& db_name);

    // ----- Tables -----

    void
    create_table(const MetaTable& table);

    bool
    exists_table(const std::string& name);
    bool
    exists_table(const std::string& name, const std::string& db_name);

    const MetaTable&
    get_table(const std::string& name);
    const MetaTable&
    get_table(const std::string& name, const std::string& db_name);
    const MetaTable&
    get_table_by_id(const std::string& id) const;

    void
    update_table(const MetaTable& new_table);

    // ----- Data -----

    void
    Storage::insert_row(MetaTable& table, DataRow row) {
        wal_.push_insert(table, row);
        page_buffers_.insert_row(table, row);
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
}