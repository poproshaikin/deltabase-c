#include "include/registry.hpp"

#include <stdexcept>
#include <iostream>
#include <format>

extern "C" {
#include "../../core/include/core.h"
#include "../../core/include/utils.h"
#include "../../core/include/meta.h"
}

namespace catalog {

    MetaRegistrySnapshot::MetaRegistrySnapshot(
        std::unordered_map<std::string, CppMetaTable> tables,
        std::unordered_map<std::string, CppMetaSchema> schemas,
        std::unordered_map<std::string, CppMetaColumn> columns
    )
        : tables(tables), schemas(schemas), columns(columns), captured_time(time(nullptr)) {
    }

    bool
    MetaRegistrySnapshot::operator==(const MetaRegistrySnapshot& other) const {
        return 
            tables  == other.tables &&
            schemas == other.schemas &&
            columns == other.columns;
    }

    MetaRegistrySnapshotDiff
    MetaRegistrySnapshot::diff(const MetaRegistrySnapshot& other) const {
        const auto& older = captured_time > other.captured_time ? *this : other;
        const auto& newer = captured_time < other.captured_time ? *this : other;

        std::unordered_map<std::string, CppMetaTable> removed_tables;
        std::unordered_map<std::string, CppMetaColumn> removed_columns;
        std::unordered_map<std::string, CppMetaSchema> removed_schemas;

        for (const auto& [old_table_id, old_table] : older.tables) {
            if (!newer.tables.contains(old_table_id)) {
                removed_tables[old_table.id] = old_table;
            }
        }

        for (const auto& [old_column_id, old_column] : older.columns) {
            if (!newer.columns.contains(old_column_id)) {
                removed_columns[old_column_id] = old_column;
            }
        }

        for (const auto& [old_schema_id, old_schema] : older.schemas) {
            if (!newer.schemas.contains(old_schema.id)) {
                removed_schemas[old_schema.id] = old_schema;
            }
        }

        std::unordered_map<std::string, CppMetaTable> added_tables;
        std::unordered_map<std::string, CppMetaColumn> added_columns;
        std::unordered_map<std::string, CppMetaSchema> added_schemas;

        for (const auto& [new_table_id, new_table] : newer.tables) {
            if (!older.tables.contains(new_table_id)) {
                added_tables[new_table_id] = new_table;
            }
        }

        for (const auto& [new_column_id, new_column] : newer.columns) {
            if (!older.columns.contains(new_column_id)) {
                added_columns[new_column_id] = new_column;
            }
        }

        for (const auto& [new_schema_id, new_schema] : newer.schemas) {
            if (!older.schemas.contains(new_schema.id)) {
                added_schemas[new_schema_id] = new_schema;
            }
        }

        std::unordered_map<std::string, CppMetaTable> updated_tables;
        std::unordered_map<std::string, CppMetaColumn> updated_columns;
        std::unordered_map<std::string, CppMetaSchema> updated_schemas;

        for (const auto& [new_table_id, new_table] : newer.tables) {
            if (older.tables.contains(new_table_id)) {
                const auto& older_table = older.tables.at(new_table_id);
                if (!older_table.compare_content(new_table)) {
                    updated_tables[new_table.id] = new_table;
                }
            }
        }

        for (const auto& [new_column_id, new_column] : newer.columns) {
            if (older.columns.contains(new_column_id)) {
                const auto& older_column = older.columns.at(new_column_id);
                if (!older_column.compare_content(new_column)) {
                    updated_columns[new_column.id] = new_column;
                }
            }
        }

        for (const auto& [new_schema_id, new_schema] : newer.schemas) {
            if (older.schemas.contains(new_schema.id)) {
                const auto& older_schema = older.schemas.at(new_schema.id);
                if (!older_schema.compare_content(new_schema)) {
                    updated_schemas[new_schema.id] = new_schema;
                }
            }
        }

        MetaRegistrySnapshotDiff result;
        result.added_tables    = std::move(added_tables);
        result.added_columns   = std::move(added_columns);
        result.added_schemas   = std::move(added_schemas);
        result.updated_tables  = std::move(updated_tables);
        result.updated_columns = std::move(updated_columns);
        result.updated_schemas = std::move(updated_schemas);
        result.removed_tables  = std::move(removed_tables);
        result.removed_columns = std::move(removed_columns);
        result.removed_schemas = std::move(removed_schemas);

        return result;
    }

    void
    MetaRegistry::upload() {  
        auto current = make_snapshot();
        auto diff = current.diff(initial_snapshot_);

        for (const auto& added_schema : schemas_) {
            
        }
    }   

    void
    MetaRegistry::download() {
        if (ensure_fs_initialized() != 0) {
            throw std::runtime_error("Failed to initialize fs");
        }

        char buffer[PATH_MAX];

        if (get_executable_dir(buffer, PATH_MAX) != 0) {
            throw std::runtime_error("Failed to get executable directory");
        }

        size_t databases_count = 0;
        char** databases = get_databases(&databases_count);

        if (!databases || databases_count == 0) {
            // No databases exist yet, that's fine
            return;
        }

        for (size_t i = 0; i < databases_count; i++) {
            size_t schemas_count = 0;
            char** schemas = ::get_schemas(databases[i], &schemas_count);
            if (!schemas || schemas_count == 0) {
                // Database exists but has no schemas, that's suspicious but fine
                continue;
            }

            for (size_t j = 0; j < schemas_count; j++) {
                MetaSchema c_schema;
                if (::get_schema(databases[i], schemas[j], &c_schema) != 0) {
                    throw std::runtime_error("Failed to get meta schema");
                }
                auto schema = CppMetaSchema::from_c(c_schema);
                this->add_schema(std::move(schema));

                size_t tables_count = 0;
                char** tables = ::get_tables(databases[i], schemas[j], &tables_count);
                if (!tables || tables_count == 0) {
                    continue;
                }

                for (size_t k = 0; k < tables_count; k++) {
                    MetaTable c_table;
                    if (::get_table(databases[i], schemas[j], tables[k], &c_table) != 0) {
                        throw std::runtime_error("Failed to get meta table");
                    }

                    auto table = CppMetaTable::from_c(c_table);

                    for (const auto& column : table.columns) {
                        this->add_column(column);
                    }

                    this->add_table(std::move(table));
                    free(tables[k]);
                    CppMetaTable::cleanup_c(c_table);
                }
                free(tables);
                free(schemas[j]);
                CppMetaSchema::cleanup_c(c_schema);
            }
            free(schemas);
            free(databases[i]);
        }
        free(databases);
    }

    MetaRegistry::MetaRegistry(engine::EngineConfig cfg) : cfg_(cfg) {
        download();
        initial_snapshot_ = make_snapshot();
    }

    MetaRegistry::~MetaRegistry() {
        upload();
    }

    MetaRegistrySnapshot
    MetaRegistry::make_snapshot() const {
        return {tables_, schemas_, columns_};
    }

    bool
    MetaRegistry::has_table(const std::string& table_name) const noexcept {
        for (const auto& table : tables_) {
            if (table.second.name == table_name && schemas_.contains(table.second.schema_id)) {
                const auto& schema = schemas_.at(table.second.schema_id);
                if (schema.name != cfg_.default_schema) {
                    return false;
                }
                return true;
            }
        }
        return false;
    }

    bool
    MetaRegistry::has_table(const std::string& table_name, const std::string& schema_name) const {
        for (const auto& table : tables_) {
            if (table.second.name == table_name && schemas_.contains(table.second.schema_id)) {
                const auto& schema = schemas_.at(table.second.schema_id);
                if (schema.name != schema_name) {
                    throw std::runtime_error(
                        std::format(
                            "In has_table: id of the schema '{}' doesnt equal to the schema id of "
                            "the table '{}'",
                            schema_name,
                            table_name
                        )
                    );
                }
                return true;
            }
        }
        return false;
    }

    bool
    MetaRegistry::has_column(
        const std::string& column_name,
        const std::string& table_name
    ) const {
        return has_column(column_name, table_name, cfg_.default_schema);
    }

    bool
    MetaRegistry::has_column(
        const std::string& column_name,
        const std::string& table_name,
        const std::string& schema_name
    ) const {
        for (const auto& column : columns_) {
            if (column.second.name == column_name && tables_.contains(column.second.table_id)) {
                const auto& table = tables_.at(column.second.table_id);
                if (table.name == table_name && schemas_.contains(table.schema_id)) {
                    const auto& schema = schemas_.at(table.schema_id);
                    if (schema.name != schema_name) {
                        throw std::runtime_error( // 
                            std::format(
                                "In has_table: id of the schema '{}' doesnt equal to the schema id "
                                "of "
                                "the table '{}'",
                                schema_name,
                                table_name
                            )
                        );
                    }
                    return true;
                }
            }
        }
        return false;
    }

    bool
    MetaRegistry::has_schema(const std::string& schema_name) const noexcept {
        for (const auto& schema : schemas_) {
            if (schema.second.name == schema_name) {
                return true;
            }
        }
        return false;
    }

    const CppMetaTable&
    MetaRegistry::get_table(const std::string& table_name) const {
        return get_table(table_name, cfg_.default_schema);
    }

    const CppMetaTable&
    MetaRegistry::get_table(const std::string& table_name, const std::string& schema_name) const {
        for (const auto& table : tables_) {
            if (table.second.name == table_name && schemas_.contains(table.second.schema_id)) {
                const auto& schema = schemas_.at(table.second.schema_id);
                if (schema.name != schema_name) {
                    throw std::runtime_error(
                        std::format(
                            "In get_table: id of the schema '{}' doesnt equal to the schema id of "
                            "the table '{}'",
                            schema_name,
                            table_name
                        )
                    );
                }
                return table.second;
            }
        }

        throw std::runtime_error(std::format("In get_table: failed to get table '{}'", table_name));
    }

    const CppMetaColumn&
    MetaRegistry::get_column(const std::string& column_name, const std::string& table_name) const {
        return get_column(column_name, table_name, cfg_.default_schema);
    }

    const CppMetaColumn&
    MetaRegistry::get_column(
        const std::string& column_name,
        const std::string& table_name,
        const std::string& schema_name
    ) const {
        for (const auto& column : columns_) {
            if (column.second.name == column_name && tables_.contains(column.second.table_id)) {
                const auto& table = tables_.at(column.second.table_id);
                if (table.name == table_name && schemas_.contains(table.schema_id)) {
                    const auto& schema = schemas_.at(table.schema_id);
                    if (schema.name != schema_name) {
                        throw std::runtime_error( // 
                            std::format(
                                "In has_table: id of the schema '{}' doesnt equal to the schema id "
                                "of "
                                "the table '{}'",
                                schema_name,
                                table_name
                            )
                        );
                    }
                    return column.second;
                }
            }
        }

        throw std::runtime_error(
            std::format(
                "In get_column: failed to get a column '{}' of the table '{}' in the schema '{}'",
                column_name,
                table_name,
                schema_name
            )
        );
    }

    const CppMetaSchema&
    MetaRegistry::get_schema(const std::string& schema_name) const {
        for (const auto& schema : schemas_) {
            if (schema.second.name == schema_name) {
                return schema.second;
            }
        }

        throw std::runtime_error("In get_schema: failed to get schema '" + schema_name + "'");
    }

    void
    MetaRegistry::add_table(const CppMetaTable& table) {
        if (!schemas_.contains(table.schema_id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::add_table: schema '{}' doesn't exist", table.schema_id
                )
            );
        }

        if (tables_.contains(table.id) && tables_.at(table.id).name == table.name) {
            throw std::runtime_error(
                std::format(
                    "Database integrity violation: duplicate table '{}' with ID '{}'. "
                    "This should not happen and indicates corrupted metadata - "
                    "there are duplicate schema names in the database",
                    table.name,
                    table.id
                )
            );
        }

        for (const auto& col : table.columns) {
            add_column(col);
        }

        tables_[table.id] = table;
    }

    void
    MetaRegistry::add_schema(const CppMetaSchema& schema) {
        if (schemas_.contains(schema.id)) {
            throw std::runtime_error(
                std::format(
                    "Database integrity violation: duplicate schema '{}' with ID '{}'. "
                    "This should not happen and indicates corrupted metadata",
                    schema.name,
                    schema.id
                )
            );
        }

        schemas_[schema.id] = schema;
    }

    void
    MetaRegistry::add_column(const CppMetaColumn& column) {
        if (!tables_.contains(column.table_id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::add_column: table with ID '{}' doesn't exist, "
                    "cannot add column '{}'",
                    column.table_id,
                    column.name
                )
            );
        }

        if (columns_.contains(column.id)) {
            throw std::runtime_error(
                std::format(
                    "Database integrity violation: duplicate column '{}' with ID '{}'. "
                    "This should not happen and indicates corrupted metadata",
                    column.name,
                    column.id
                )
            );
        }

        columns_[column.id] = column;
    }

    void
    MetaRegistry::update_table(const CppMetaTable& updated) {
        if (!tables_.contains(updated.id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::update_table: table with ID '{}' doesn't exist",
                    updated.id
                )
            );
        }

        // Update columns first
        for (const auto& col : updated.columns) {
            if (columns_.contains(col.id)) {
                columns_[col.id] = col;
            } else {
                add_column(col);
            }
        }

        tables_[updated.id] = updated;
    }

    void
    MetaRegistry::update_column(const CppMetaColumn& updated) {
        if (!columns_.contains(updated.id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::update_column: column with ID '{}' doesn't exist",
                    updated.id
                )
            );
        }

        if (!tables_.contains(updated.table_id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::update_column: table with ID '{}' doesn't exist",
                    updated.table_id
                )
            );
        }

        columns_[updated.id] = updated;
    }

    void
    MetaRegistry::update_schema(const CppMetaSchema& updated) {
        if (!schemas_.contains(updated.id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::update_schema: schema with ID '{}' doesn't exist",
                    updated.id
                )
            );
        }

        schemas_[updated.id] = updated;
    }

    void
    MetaRegistry::remove_table(const CppMetaTable& table) {
        if (!tables_.contains(table.id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::remove_table: table with ID '{}' doesn't exist",
                    table.id
                )
            );
        }

        // Remove all columns of this table first
        auto it = columns_.begin();
        while (it != columns_.end()) {
            if (it->second.table_id == table.id) {
                it = columns_.erase(it);
            } else {
                ++it;
            }
        }

        tables_.erase(table.id);
    }

    void
    MetaRegistry::remove_column(const CppMetaColumn& column) {
        if (!columns_.contains(column.id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::remove_column: column with ID '{}' doesn't exist",
                    column.id
                )
            );
        }

        columns_.erase(column.id);
    }

    void
    MetaRegistry::remove_schema(const CppMetaSchema& schema) {
        if (!schemas_.contains(schema.id)) {
            throw std::runtime_error(
                std::format(
                    "In MetaRegistry::remove_schema: schema with ID '{}' doesn't exist",
                    schema.id
                )
            );
        }

        // Check if there are any tables in this schema
        for (const auto& table : tables_) {
            if (table.second.schema_id == schema.id) {
                throw std::runtime_error(
                    std::format(
                        "In MetaRegistry::remove_schema: cannot remove schema '{}' - "
                        "it contains table '{}'. Remove tables first",
                        schema.name,
                        table.second.name
                    )
                );
            }
        }

        schemas_.erase(schema.id);
    }

    void
    MetaRegistry::upload_table(const CppMetaTable& table) {
        const auto& schema = schemas_[table.schema_id];
        MetaTable c_table = table.to_c();

        if (::update_table(schema.db_name.c_str(), schema.name.c_str(), &c_table) != 0) {
            throw std::runtime_error(std::format("In upload_table: failed to update table '{}'", table.name));
        }

        CppMetaTable::cleanup_c(c_table);
    }

    void
    MetaRegistry::upload_schema(const CppMetaSchema& schema) {
        MetaSchema c_schema = schema.to_c();

        if (::update_schema(&c_schema) != 0) {
            throw std::runtime_error(std::format("In upload_schema: failed to update schema '{}'", schema.name));
        }

        CppMetaSchema::cleanup_c(c_schema);
    }
} // namespace catalog