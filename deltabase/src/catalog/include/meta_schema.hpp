#pragma once 

#include <optional>
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

        CppMetaColumn() = default;

        CppMetaColumn(std::string name, DataType type, DataColumnFlags flags)
            : name(name), data_type(type), flags(flags) {
            uuid_generate_time(this->id);
        }

        CppMetaColumn(std::string name, DataType type, DataColumnFlags flags, const uuid_t& table_id)
            : CppMetaColumn(name, type, flags) {
                memcpy(this->table_id, table_id, sizeof(uuid_t));
            }

        CppMetaColumn(MetaColumn column)
            : id(), name(column.name ? column.name : ""), 
              data_type(column.data_type), flags(column.flags) {
            memcpy(this->id, column.id, sizeof(uuid_t));
            memcpy(this->table_id, column.table_id, sizeof(uuid_t));
        }

        ~CppMetaColumn() = default;

        std::string
        get_id() const override {
            return std::string(reinterpret_cast<const char*>(this->id));
        }

        std::string
        get_name() const override {
            return this->name;
        }

        DataType
        get_data_type() const {
            return this->data_type;
        }

        MetaColumn
        create_meta_column() const;

        std::string
        to_string() const override;

        operator MetaColumn() const { return this->create_meta_column(); }

    private:
        uuid_t id;
        uuid_t table_id;
        std::string name;
        DataType data_type;
        DataColumnFlags flags;
    };

    struct CppMetaTable : public CppMetaSchemaWrapper {

        CppMetaTable() = default;

        CppMetaTable(std::string name, std::optional<std::string> schema_name = std::nullopt)
            : table_name(name), schema_name(schema_name), has_pk(false), last_rid(0) {
            uuid_generate_time(this->table_id);
        }

        CppMetaTable(MetaTable table);
        ~CppMetaTable() = default; // No manual cleanup needed

        std::string
        get_id() 
        const override {
            return std::string(reinterpret_cast<const char*>(table_id));
        }
        
        std::string
        get_name() const override {
            return this->table_name;
        }

        uint64_t 
        get_columns_count() const {
            return this->columns.size();
        }

        const CppMetaColumn&
        get_column(const std::string& name) const;

        const std::vector<CppMetaColumn>&
        get_columns() const {
            return this->columns;
        }

        void
        add_column(const CppMetaColumn& col) {
            this->columns.push_back(col);
        }

        bool
        has_column(const std::string& col_name) const;

        MetaTable
        create_meta_table() const;

        std::string
        to_string() const override;

        operator MetaTable() const { return this->create_meta_table(); }

    private:

        uuid_t table_id;
        uint64_t last_rid;

        std::optional<std::string> schema_name;
        std::string table_name;

        bool has_pk;
        uuid_t pk;

        std::vector<CppMetaColumn> columns;

        std::vector<CppMetaColumn>
        parse_columns(const MetaTable& table) const;

        void
        cleanup_original_table(const MetaTable& table) const;
    };
}