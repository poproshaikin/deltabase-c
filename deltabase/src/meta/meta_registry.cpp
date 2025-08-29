#include "include/meta_registry.hpp"
#include "../converter/include/converter.hpp"
#include "../misc/include/utils.hpp"
#include <linux/limits.h>
#include <iostream>

extern "C" {
#include "../core/include/core.h"
#include "../core/include/utils.h"
}

namespace meta {
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

    MetaColumn
    create_meta_column(const std::string& name, DataType type, DataColumnFlags flags) {
        MetaColumn column;

        column.name = make_c_string(name);
        column.data_type = type;
        column.flags = flags;
        uuid_generate_time(column.id);

        return column;
    }

    MetaTable
    create_meta_table(const std::string& name,
                                    const std::vector<sql::ColumnDefinition> col_defs) {
        MetaTable table;

        table.name = make_c_string(name);
        table.columns = make_c_ptr_arr(converter::convert_defs_to_mcs(col_defs));
        table.columns_count = col_defs.size();
        table.has_pk = false;
        table.last_rid = 0;
        uuid_generate_time(table.id);

        return table;
    }

    void
    cleanup_meta_table(MetaTable& table) {
        if (table.name) {
            free(table.name);
        }
        if (table.columns) {
            for (uint64_t i = 0; i < table.columns_count; i++) {
                if (table.columns[i]) {
                    free_col(table.columns[i]);
                }
            }
            free(table.columns);
        }

    }

    void
    cleanup_meta_column(MetaColumn& column) {
        if (column.name) {
            free(column.name);
        }
    }

    void
    cleanup_meta_table(MetaTable* table) {
        if (!table)
            return;

        cleanup_meta_table(*table);
        free(table);

    }

    void
    cleanup_meta_column(MetaColumn* column) {
        if (!column)
            return;

        cleanup_meta_column(*column);
        free(column);
    }
} // namespace meta