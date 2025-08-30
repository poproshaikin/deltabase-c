#pragma once 

#include <string>
#include <vector>

extern "C" {
#include "../../core/include/meta.h"
}

namespace catalog {

    struct CppMetaSchemaWrapper
    {
        virtual ~CppMetaSchemaWrapper() = 0;

        virtual std::string
        get_id() const = 0;

        virtual std::string
        get_name() const = 0;

        virtual std::string
        to_string() const = 0;
    };

    struct CppMetaColumn : public CppMetaSchemaWrapper {

        CppMetaColumn(MetaColumn column) : column_id(), name_copy(column.name ? column.name : ""), 
                                           data_type(column.data_type), flags(column.flags) {
            memcpy(column_id, column.id, sizeof(column_id));
        }

        ~CppMetaColumn() = default; // No manual cleanup needed now

        // Delete copy constructor and assignment to prevent issues
        CppMetaColumn(const CppMetaColumn&) = delete;
        CppMetaColumn& operator=(const CppMetaColumn&) = delete;

        // Enable move semantics
        CppMetaColumn(CppMetaColumn&& other) noexcept 
            : name_copy(std::move(other.name_copy)), data_type(other.data_type), flags(other.flags) {
            memcpy(column_id, other.column_id, sizeof(column_id));
        }

        CppMetaColumn& operator=(CppMetaColumn&& other) noexcept {
            if (this != &other) {
                memcpy(column_id, other.column_id, sizeof(column_id));
                name_copy = std::move(other.name_copy);
                data_type = other.data_type;
                flags = other.flags;
            }
            return *this;
        }

        std::string
        get_id() const override {
            return std::string(reinterpret_cast<const char*>(column_id));
        }

        std::string
        get_name() const override {
            return name_copy;
        }

        MetaColumn
        create_meta_column() const;

        std::string
        to_string() const override;

        operator MetaColumn() const { return this->create_meta_column(); }
    private:

        uuid_t column_id;
        std::string name_copy;
        DataType data_type;
        DataColumnFlags flags;
    };

    struct CppMetaTable : public CppMetaSchemaWrapper {

        CppMetaTable(MetaTable table) : table_id(), name_copy(table.name ? table.name : ""),
                                        has_pk(table.has_pk), last_rid(table.last_rid),
                                        columns_count(table.columns_count), columns(parse_columns(table)) {
            memcpy(table_id, table.id, sizeof(table_id));
            if (table.has_pk) {
                memcpy(pk, table.pk, sizeof(pk));
            }
            
            // Now we can safely free the original C struct
            cleanup_original_table(table);
        }

        ~CppMetaTable() = default; // No manual cleanup needed

        // Delete copy constructor and assignment  
        CppMetaTable(const CppMetaTable&) = delete;
        CppMetaTable& operator=(const CppMetaTable&) = delete;

        // Enable move semantics
        CppMetaTable(CppMetaTable&& other) noexcept 
            : name_copy(std::move(other.name_copy)), has_pk(other.has_pk), last_rid(other.last_rid),
              columns_count(other.columns_count), columns(std::move(other.columns)) {
            memcpy(table_id, other.table_id, sizeof(table_id));
            if (has_pk) {
                memcpy(pk, other.pk, sizeof(pk));
            }
        }

        CppMetaTable& operator=(CppMetaTable&& other) noexcept {
            if (this != &other) {
                memcpy(table_id, other.table_id, sizeof(table_id));
                name_copy = std::move(other.name_copy);
                has_pk = other.has_pk;
                last_rid = other.last_rid;
                columns_count = other.columns_count;
                columns = std::move(other.columns);
                if (has_pk) {
                    memcpy(pk, other.pk, sizeof(pk));
                }
            }
            return *this;
        }

        std::string
        get_id() const override {
            return std::string(reinterpret_cast<const char*>(table_id));
        }
        
        std::string
        get_name() const override {
            return name_copy;
        }

        const std::vector<CppMetaColumn>&
        get_columns() const {
            return this->columns;
        }

        MetaTable
        create_meta_table() const;

        std::string
        to_string() const override;

        operator MetaTable() const { return this->create_meta_table(); }

    private:

        uuid_t table_id;
        std::string name_copy;
        bool has_pk;
        uuid_t pk;
        uint64_t columns_count;
        uint64_t last_rid;
        std::vector<CppMetaColumn> columns;

        std::vector<CppMetaColumn>
        parse_columns(const MetaTable& table) const;
        
        void cleanup_original_table(const MetaTable& table) const;
    };

    
}