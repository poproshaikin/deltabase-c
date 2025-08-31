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

            size_t tables_count = 0;
            char** tables = ::get_tables(databases[i], &tables_count);
            if (!tables || tables_count == 0) {
                // Database exists but has no tables, that's fine
                continue;
            }

            for (size_t j = 0; j < tables_count; j++) {
                MetaTable raw_table;
                if (::get_table(databases[i], tables[j], &raw_table) != 0) {
                    throw std::runtime_error("Failed to get meta table");
                }

                CppMetaTable table(raw_table);
                this->add_schema(std::move(table));

                free(tables[j]);
            }

            free(tables);
        }

        for (size_t i = 0; i < databases_count; i++) {
            free(databases[i]);
        }
        free(databases);
    }

    std::optional<std::shared_ptr<CppMetaSchemaWrapper>>
    MetaRegistry::get_schema(const std::string& id) const {
        auto it = this->registry.find(id);
        if (it != this->registry.end()) {
            return it->second;
        }
        return std::nullopt;
    }


    bool
    MetaRegistry::has_table(const std::string& name) const {
        for (const auto& kvp : this->registry) {
            if (kvp.first == name) {
                auto table = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
                if (table) {
                    return true;
                }
            }
        }
        return false;
    }

    bool
    MetaRegistry::has_table(const sql::TableIdentifier& identifier) const {
        return this->has_table(identifier.table_name.value);
    }

    bool
    MetaRegistry::has_virtual_table(const sql::TableIdentifier& identifier) const {
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


    CppMetaTable
    MetaRegistry::get_table(const std::string& table_name) const {
        for (const auto& kvp : this->registry) {
            if (kvp.second->get_name() == table_name) {
                std::shared_ptr<CppMetaTable> table = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
                if (table) {
                    return *table;
                }
            }
        }

        throw TableDoesntExist(table_name);
    }

    CppMetaTable
    MetaRegistry::get_table(const sql::SqlToken& table) const {
        return this->get_table(table.value);
    }

    CppMetaTable
    MetaRegistry::get_table(const sql::TableIdentifier& identifier) const {
        for (const auto& kvp : this->registry) {
            if (kvp.second->get_name() == identifier.table_name.value) {
                std::shared_ptr<CppMetaTable> table = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
                if (table) {
                    return *table;
                }
            }
        }

        throw TableDoesntExist(identifier.table_name.value);
    }

    CppMetaTable
    MetaRegistry::get_virtual_table(const sql::TableIdentifier& identifier) const {
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
        static_assert(std::is_base_of_v<CppMetaSchemaWrapper, std::decay_t<T>>,
                      "T must derive from CppMetaSchemaWrapper");
        registry[schema.get_id()] = std::make_unique<std::decay_t<T>>(std::forward<T>(schema));
    }

    std::vector<CppMetaTable>
    MetaRegistry::get_tables() const {
        std::vector<CppMetaTable> result;

        for (const auto& kvp : this->registry) {
            auto table_ptr = std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
            if (table_ptr) {
                result.push_back(*table_ptr);
            }
        }
        
        return result;
    }

    std::vector<CppMetaColumn>
    MetaRegistry::get_columns() const {
        std::vector<CppMetaColumn> result;

        for (const auto& kvp : this->registry) {
            auto column_ptr = std::dynamic_pointer_cast<CppMetaColumn>(kvp.second);
            if (column_ptr) {
                result.push_back(*column_ptr);
            }
        }

        return result;
    }

    bool
    is_table_virtual(const sql::TableIdentifier& table) {
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
} // namespace catalog