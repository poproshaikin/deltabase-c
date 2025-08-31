#include "include/meta_schema.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

namespace catalog {
    CppMetaSchemaWrapper::~CppMetaSchemaWrapper() {
    }

    std::vector<CppMetaColumn>
    CppMetaTable::parse_columns(const MetaTable& table) const {
        std::vector<CppMetaColumn> columns;
        columns.reserve(table.columns_count);

        for (size_t i = 0; i < table.columns_count; i++) {
            columns.emplace_back(table.columns[i]);
        }

        return columns;
    }

    std::string
    CppMetaColumn::to_string() const {
        std::ostringstream ss;
        ss << this->get_name() << " (ID: " << this->get_id() << ")" << std::endl;
        return ss.str();
    }

    std::string
    CppMetaTable::to_string() const {
        std::ostringstream ss;
        ss << "=== Table Information ===" << std::endl;
        ss << "ID: " << get_id() << std::endl;
        ss << "Name: " << get_name() << std::endl;
        ss << "Columns count: " << columns.size() << std::endl;

        if (!columns.empty()) {
            std::cout << "\n--- Columns ---" << std::endl;
            for (size_t i = 0; i < columns.size(); i++) {
                ss << columns[i].to_string();
            }
        }

        ss << "=========================" << std::endl;
        return ss.str();
    }

    void
    CppMetaTable::cleanup_original_table(const MetaTable& table) const {
        if (table.name) {
            free(table.name);
        }
        if (table.columns) {
            for (uint64_t i = 0; i < table.columns_count; i++) {
                if (table.columns[i].name) {
                    free(table.columns[i].name);
                }
            }
            free(table.columns);
        }
    }

    MetaColumn
    CppMetaColumn::create_meta_column() const {
        MetaColumn column;

        // Copy UUID
        memcpy(column.id, column_id, sizeof(column.id));

        // Allocate and copy name
        column.name = static_cast<char*>(malloc(name_copy.length() + 1));
        if (column.name) {
            strcpy(column.name, name_copy.c_str());
        }

        // Copy other fields
        column.data_type = data_type;
        column.flags = flags;

        return column;
    }

    MetaTable
    CppMetaTable::create_meta_table() const {
        MetaTable table;

        // Copy UUID
        memcpy(table.id, table_id, sizeof(table.id));

        // Allocate and copy name
        table.name = static_cast<char*>(malloc(name_copy.length() + 1));
        if (table.name) {
            strcpy(table.name, name_copy.c_str());
        }

        // Copy other fields
        table.has_pk = has_pk;
        if (has_pk) {
            memcpy(table.pk, pk, sizeof(table.pk->id));
        }
        table.columns_count = columns.size();
        table.last_rid = last_rid;

        // Allocate and copy columns array
        if (columns.size() > 0) {
            table.columns = static_cast<MetaColumn*>(malloc(columns.size() * sizeof(MetaColumn)));
            if (table.columns) {
                for (size_t i = 0; i < columns.size(); i++) {
                    table.columns[i] = columns[i].create_meta_column();
                }
            }
        } else {
            table.columns = nullptr;
        }

        return table;
    }
} // namespace catalog