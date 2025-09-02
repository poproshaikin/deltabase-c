#include <string>
#include <vector>
#include "../../sql/include/parser.hpp"

extern "C" {
#include "../../core/include/meta.h"
}

namespace catalog::models {

    auto
    create_meta_column(const std::string& name, DataType type, DataColumnFlags flags) -> MetaColumn;

    auto
    create_meta_table(const std::string& name, const std::vector<sql::ColumnDefinition> col_defs) -> MetaTable;

    auto
    create_meta_schema(const std::string& name) -> MetaSchema;

    void 
    cleanup_meta_schema(MetaSchema& schema);

    void
    cleanup_meta_table(MetaTable& table);

    void
    cleanup_meta_column(MetaColumn& column);

    void
    cleanup_meta_table(MetaTable* table);

    void
    cleanup_meta_column(MetaColumn* column);
}