#include "include/meta_object.hpp"
#include "../misc/include/exceptions.hpp"
#include "../misc/include/utils.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

namespace catalog {
    CppMetaObjectWrapper::~CppMetaObjectWrapper() = default;

    CppMetaTable::CppMetaTable(std::string name, std::optional<std::string> schema_name)
        : table_name_(std::move(name)), schema_name_(std::move(schema_name)), has_pk_(false),
          last_rid_(0) {
        uuid_generate_time(table_id_);
    }

    auto
    CppMetaTable::parse_columns(const MetaTable& table) const -> std::vector<CppMetaColumn> {
        std::vector<CppMetaColumn> columns;
        columns.reserve(table.columns_count);

        for (size_t i = 0; i < table.columns_count; i++) {
            columns.emplace_back(table.columns[i]);
        }

        return columns;
    }

    auto
    CppMetaColumn::to_string() const -> std::string {
        std::ostringstream ss;
        ss << this->get_name() << " (ID: " << this->get_id() << ")" << std::endl;
        return ss.str();
    }

    auto
    CppMetaTable::to_string() const -> std::string {
        std::ostringstream ss;
        ss << "=== Table Information ===" << std::endl;
        ss << "ID: " << get_id() << std::endl;
        ss << "Name: " << get_name() << std::endl;
        ss << "Columns count: " << columns_.size() << std::endl;

        if (!columns_.empty()) {
            std::cout << "\n--- Columns ---" << std::endl;
            for (const auto & column : columns_) {
                ss << column.to_string();
            }
        }

        ss << "=========================" << std::endl;
        return ss.str();
    }

    auto
    CppMetaColumn::create_meta_column() const -> MetaColumn {
        MetaColumn column;

        memcpy(column.id, id_, sizeof(column.id));
        memcpy(column.table_id, table_id_, sizeof(column.id));

        column.name = static_cast<char*>(malloc(name_.length() + 1));
        if (column.name) {
            strcpy(column.name, name_.c_str());
        }

        column.data_type = data_type_;
        column.flags = flags_;

        return column;
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

    auto
    CppMetaTable::create_meta_table() const -> MetaTable {
        MetaTable table;

        // Copy UUID
        memcpy(table.id, table_id_, sizeof(table.id));

        // Allocate and copy name
        table.name = static_cast<char*>(malloc(table_name_.length() + 1));
        if (table.name) {
            strcpy(table.name, table_name_.c_str());
        }

        // Copy other fields
        table.has_pk = has_pk_;
        if (has_pk_) {
            memcpy(table.pk, pk_, sizeof(table.pk->id));
        }
        table.columns_count = columns_.size();
        table.last_rid = last_rid_;

        // Allocate and copy columns array
        if (columns_.size() > 0) {
            table.columns = static_cast<MetaColumn*>(malloc(columns_.size() * sizeof(MetaColumn)));
            if (table.columns) {
                for (size_t i = 0; i < columns_.size(); i++) {
                    table.columns[i] = columns_[i].create_meta_column();
                }
            }
        } else {
            table.columns = nullptr;
        }

        return table;
    }

    CppMetaTable::CppMetaTable(MetaTable table)
        : table_id_(), table_name_(table.name ? table.name : ""), has_pk_(table.has_pk),
          last_rid_(table.last_rid), columns_(parse_columns(table)) {
        memcpy(table_id_, table.id, sizeof(table_id_));
        if (table.has_pk) {
            memcpy(pk_, table.pk, sizeof(pk_));
        }

        // Now we can safely free the original C struct
        cleanup_original_table(table);
    }

    auto
    CppMetaTable::get_column(const std::string& name) const -> const CppMetaColumn& {
        for (const CppMetaColumn& col : columns_) {
            if (col.get_name() == name) {
                return col;
            }
        }

        throw ColumnDoesntExists(name);
    }

    auto
    CppMetaTable::has_column(const std::string& col_name) const -> bool {
        for (const auto& col : columns_) {
            if (col.get_name() == col_name) {
                return true;
            }
        }

        return false;
    }

    CppMetaSchema::CppMetaSchema(const MetaSchema& schema) : name_(std::string(schema.name)) {
        memcpy(id_, schema.name, sizeof(uuid_t));
    }

    CppMetaSchema::CppMetaSchema(std::string name) : name_(std::move(name)) {
        uuid_generate_time(id_);
    }

    auto
    CppMetaSchema::get_id() const -> std::string {
        char uuid_str[37]; 
        uuid_unparse(id_, uuid_str);
        return std::string(uuid_str);
    }

    auto
    CppMetaSchema::get_name() const -> std::string {
        return name_;
    }

    auto
    CppMetaSchema::to_string() const -> std::string {
        char uuid_str[37];
        uuid_unparse(id_, uuid_str);
        return "Schema (ID: " + std::string(uuid_str) + ", Name: " + name_ + ")";
    }

    auto
    CppMetaSchema::to_meta_schema() const -> MetaSchema {
        MetaSchema schema;

        schema.name = make_c_string(name_);
        memcpy(schema.id, id_, sizeof(uuid_t));
        
        return schema;
    }
} // namespace catalog