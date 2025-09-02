#pragma once 

#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include "../../core/include/meta.h"
}

namespace catalog {

    struct CppMetaObjectWrapper
    {
        virtual ~CppMetaObjectWrapper() = 0;

        [[nodiscard]] virtual auto
        get_id() const -> std::string = 0;

        [[nodiscard]] virtual auto
        get_name() const -> std::string = 0;

        [[nodiscard]] virtual auto
        to_string() const -> std::string = 0;
    };

    struct CppMetaSchema : public CppMetaObjectWrapper {
        CppMetaSchema() = default;
        CppMetaSchema(const MetaSchema& schema);
        CppMetaSchema(std::string name);

        [[nodiscard]] auto
        get_id() const -> std::string override;

        [[nodiscard]] auto
        get_name() const -> std::string override;

        [[nodiscard]] auto
        to_string() const -> std::string override;

        [[nodiscard]] auto
        to_meta_schema() const -> MetaSchema;

    private:
        uuid_t id_;
        std::string name_;
    };

    struct CppMetaColumn : public CppMetaObjectWrapper {

        CppMetaColumn() = default;

        CppMetaColumn(std::string name, DataType type, DataColumnFlags flags)
            : name_(std::move(name)), data_type_(type), flags_(flags) {
            uuid_generate_time(this->id_);
        }

        CppMetaColumn(std::string name, DataType type, DataColumnFlags flags, const uuid_t& table_id)
            : CppMetaColumn(name, type, flags) {
                memcpy(this->table_id_, table_id, sizeof(uuid_t));
            }

        CppMetaColumn(const MetaColumn& column)
            : id_(), name_(column.name ? column.name : ""), 
              data_type_(column.data_type), flags_(column.flags) {
            memcpy(this->id_, column.id, sizeof(uuid_t));
            memcpy(this->table_id_, column.table_id, sizeof(uuid_t));
        }

        ~CppMetaColumn() override = default;

        [[nodiscard]] auto
        get_id() const -> std::string override {
            return std::string(reinterpret_cast<const char*>(this->id_));
        }

        [[nodiscard]] auto
        get_name() const -> std::string override {
            return this->name_;
        }

        [[nodiscard]] auto
        get_data_type() const -> DataType {
            return this->data_type_;
        }

        [[nodiscard]] auto
        create_meta_column() const -> MetaColumn;

        [[nodiscard]] auto
        to_string() const -> std::string override;

        operator MetaColumn() const { return this->create_meta_column(); }

    private:
        uuid_t id_;
        uuid_t table_id_;
        std::string name_;
        DataType data_type_;
        DataColumnFlags flags_;
    };

    struct CppMetaTable : public CppMetaObjectWrapper {

        CppMetaTable() = default;
        CppMetaTable(std::string name, std::optional<std::string> schema_name = std::nullopt);
        CppMetaTable(MetaTable table);
        ~CppMetaTable() override = default; // No manual cleanup needed

        [[nodiscard]] auto
        get_id() 
        const -> std::string override {
            return std::string(reinterpret_cast<const char*>(table_id_));
        }
        
        [[nodiscard]] auto
        get_name() const -> std::string override {
            return this->table_name_;
        }

        [[nodiscard]] auto 
        get_columns_count() const -> uint64_t {
            return this->columns_.size();
        }

        [[nodiscard]] auto
        get_column(const std::string& name) const -> const CppMetaColumn&;

        [[nodiscard]] auto
        get_columns() const -> const std::vector<CppMetaColumn>& {
            return this->columns_;
        }

        void
        add_column(const CppMetaColumn& col) {
            this->columns_.push_back(col);
        }

        [[nodiscard]] auto
        has_column(const std::string& col_name) const -> bool;

        [[nodiscard]] auto
        create_meta_table() const -> MetaTable;

        [[nodiscard]] auto
        to_string() const -> std::string override;

        operator MetaTable() const { return this->create_meta_table(); }

    private:

        uuid_t table_id_;
        uint64_t last_rid_;

        std::optional<std::string> schema_name_;
        std::string table_name_;

        bool has_pk_;
        uuid_t pk_;

        std::vector<CppMetaColumn> columns_;

        [[nodiscard]] auto
        parse_columns(const MetaTable& table) const -> std::vector<CppMetaColumn>;

        void
        cleanup_original_table(const MetaTable& table) const;
    };
}