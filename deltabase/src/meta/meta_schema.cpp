#include "include/meta_schema.hpp"

namespace meta {
    CppMetaSchemaWrapper::~CppMetaSchemaWrapper() {
    }

    std::vector<CppMetaColumn>
    CppMetaTable::parse_columns(const MetaTable& table) const {
        std::vector<CppMetaColumn> columns;
        columns.reserve(table.columns_count);

        for (size_t i = 0; i < table.columns_count; i++) {
            columns.emplace_back(*table.columns[i]);
        }

        return columns;
    }
}