#include "include/models.hpp"
#include "../misc/include/utils.hpp"
#include "../converter/include/converter.hpp"

namespace catalog::models {

    auto
    create_meta_column(const std::string& name, DataType type, DataColumnFlags flags) -> MetaColumn {
        MetaColumn column;

        column.name = make_c_string(name);
        column.data_type = type;
        column.flags = flags;
        uuid_generate_time(column.id);

        return column;
    }

    auto
    create_meta_table(const std::string& name, const std::vector<sql::ColumnDefinition> col_defs) -> MetaTable {
        MetaTable table;

        table.name = make_c_string(name);
        table.columns = make_c_arr(converter::convert_defs_to_mcs(col_defs));
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
                free_col(&table.columns[i]);
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

    void 
    cleanup_meta_schema(MetaSchema& schema) {
        free(schema.name);
    }
}