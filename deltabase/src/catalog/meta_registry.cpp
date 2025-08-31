#include "include/meta_registry.hpp"
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
            char** tables = get_tables(databases[i], &tables_count);
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