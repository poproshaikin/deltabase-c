#include "include/meta_registry.hpp"
#include "exceptions.hpp"
#include "information_schema.hpp"
#include <iostream>
#include <linux/limits.h>

extern "C" {
#include "../core/include/core.h"
#include "../core/include/utils.h"
}

namespace catalog {
    MetaRegistry::MetaRegistry() {
        init();
    }

    void
    MetaRegistry::init() {
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
            std::cout << "No databases found - registry initialized empty." << std::endl;
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
                CppMetaSchema schema(c_schema);
                this->add_schema(schema);

                size_t tables_count = 0;
                char** tables = ::get_tables(databases[i], schemas[j], &tables_count);
                if (!tables || tables_count == 0) {
                    continue;
                }

                for (size_t k = 0; k < tables_count; k++) {
                    MetaTable raw_table;
                    if (::get_table(databases[i], schemas[j], tables[k], &raw_table) != 0) {
                        throw std::runtime_error("Failed to get meta table");
                    }

                    CppMetaTable table(raw_table);
                    this->add_schema(std::move(table));
                    free(tables[k]);
                }
                free(tables);
                free(schemas[j]);
            }
            free(schemas);
            free(databases[i]);
        }
        free(databases);
    }

    template <typename T>
    auto
    MetaRegistry::has_object(const std::string& name) const -> bool {
        static_assert(std::is_base_of_v<CppMetaObjectWrapper, T>);

        for (const auto& kvp : registry_) {
            if (kvp.second->get_name() == name) {
                return true;
            }
        }

        return false;
    }

    template <typename T>
    auto
    MetaRegistry::get_object(const std::string& name) const -> const T& {
        static_assert(std::is_base_of_v<CppMetaObjectWrapper, T>);
        
        for (const auto& kvp : registry_) {
            if (kvp.second->get_name() == name) {
                std::shared_ptr<T> needed_type = std::dynamic_pointer_cast<T>(kvp.second);
                if (needed_type) {
                    return *needed_type;
                }
            }
        }

        throw std::runtime_error("Failed to get meta object with name " + name);
    }


    auto
    MetaRegistry::has_table(const std::string& name) const -> bool {
        return has_object<CppMetaTable>(name);
    }

    auto
    MetaRegistry::has_table(const sql::TableIdentifier& identifier) const -> bool {
        return this->has_table(identifier.table_name.value);
    }

    auto
    MetaRegistry::has_virtual_table(const sql::TableIdentifier& identifier) const -> bool {
        if (identifier.schema_name.has_value() &&
            identifier.schema_name.value().value == "information_schema") {
            
            const auto& table_name = identifier.table_name.value;
            if (table_name == "tables" ||
                table_name == "columns") {
                return true;
            }
        }

        return false;
    }


    auto
    MetaRegistry::get_table(const std::string& table_name) const -> CppMetaTable {
        for (const auto& kvp : registry_) {
            if (kvp.second->get_name() == table_name) {
                std::shared_ptr<CppMetaTable> table = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
                if (table) {
                    return *table;
                }
            }
        }

        throw TableDoesntExist(table_name);
    }

    auto
    MetaRegistry::get_table(const sql::SqlToken& table) const -> CppMetaTable {
        return this->get_table(table.value);
    }

    auto
    MetaRegistry::get_table(const sql::TableIdentifier& identifier) const -> CppMetaTable {
        for (const auto& kvp : registry_) {
            if (kvp.second->get_name() == identifier.table_name.value) {
                std::shared_ptr<CppMetaTable> table = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
                if (table) {
                    return *table;
                }
            }
        }

        throw TableDoesntExist(identifier.table_name.value);
    }

    auto
    MetaRegistry::get_virtual_table(const sql::TableIdentifier& identifier) const -> CppMetaTable {
        if (identifier.schema_name.has_value() && 
            identifier.schema_name.value().value == "information_schema") {
            const auto& table_name = identifier.table_name.value;

            if (table_name == "tables") {
                return catalog::information_schema::get_tables_schema();
            }
            if (table_name == "columns") {
                return catalog::information_schema::get_columns_schema();
            }
        }

        throw std::runtime_error("Failed to get virtual table " + identifier.table_name.value);
    }

    template <typename T>
    void
    MetaRegistry::add_schema(T&& schema) {
        static_assert(std::is_base_of_v<CppMetaObjectWrapper, std::decay_t<T>>,
                      "T must derive from CppMetaSchemaWrapper");
        registry_[schema.get_id()] = std::make_unique<std::decay_t<T>>(std::forward<T>(schema));
    }

    auto
    MetaRegistry::get_tables() const -> std::vector<CppMetaTable> {
        std::vector<CppMetaTable> result;

        for (const auto& kvp : registry_) {
            auto table_ptr = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
            if (table_ptr) {
                result.push_back(*table_ptr);
            }
        }
        
        return result;
    }

    auto
    MetaRegistry::get_columns() const -> std::vector<CppMetaColumn> {
        std::vector<CppMetaColumn> result;

        auto tables = this->get_tables();

        for (const auto& table : tables) {
            result.reserve(result.size() + table.get_columns_count());

            for (const auto& col : table.get_columns()) {
                result.push_back(col);
            }
        }

        return result;
    }

    auto
    is_table_virtual(const sql::TableIdentifier& table) -> bool {
        if (table.schema_name.has_value()) {
            const std::string& schema_name = table.schema_name.value().value;
            if (schema_name == "information_schema") {
                return true;
            }
        }

        const std::string& table_name = table.table_name.value;
        if (table_name == "tables" || table_name == "columns") {
            return true;
        }
        
        return false;
    }

    auto
    MetaRegistry::has_schema(const std::string& name) const -> bool {
        return has_object<CppMetaSchema>(name);
    }

    auto
    MetaRegistry::get_schema(const std::string& name) const -> const CppMetaSchema& {
        return get_object<CppMetaSchema>(name);
    }
} // namespace catalog