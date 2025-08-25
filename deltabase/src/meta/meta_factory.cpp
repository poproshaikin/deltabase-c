#include "include/meta_factory.hpp"
#include "../misc/include/utils.hpp"
#include "../converter/include/converter.hpp"

namespace meta {
    MetaColumn init_meta_column(const std::string &name, DataType type, DataColumnFlags flags) {
        MetaColumn column;
        
        column.name = make_c_string(name);
        column.data_type = type;
        column.flags = flags;
        uuid_generate_time(column.column_id);
        
        return column;
    }

    MetaTable init_meta_table(const std::string &name,
                              const std::vector<sql::ColumnDefinition> col_defs) {
        MetaTable table;

        table.name = make_c_string(name);
        table.columns = make_c_ptr_arr(converter::convert_defs_to_mcs(col_defs));
        table.columns_count = col_defs.size();
        table.has_pk = false;
        table.last_rid = 0;
        uuid_generate_time(table.table_id);
        
        return table;
    }

    void cleanup_meta_table(MetaTable& table) {
        if (table.name) {
            delete table.name;
        }
        if (table.columns) {
            for (uint64_t i = 0; i < table.columns_count; i++) {
                if (table.columns[i]) {
                    delete table.columns[i];
                }
            }
            delete table.columns;
        }
    }

    void cleanup_meta_column(MetaColumn& column) {
        if (column.name) {
            delete column.name;
        }
    }

    void cleanup_meta_table(MetaTable* table) {
        if (!table) return;

        cleanup_meta_table(*table);
        delete table;
    }

    void cleanup_meta_column(MetaColumn* column) {
        if (!column) return;

        cleanup_meta_column(*column);
        delete column;
    }
}