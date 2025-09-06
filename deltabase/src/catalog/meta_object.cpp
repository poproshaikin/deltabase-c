#include "include/meta_object.hpp"
#include "../../misc/include/utils.hpp"

namespace catalog {
    bool
    CppMetaTable::has_column(const std::string& name) {
        for (const auto& col : columns) {
            if (col.name == name) {
                return true;
            }
        }
        return false;
    }
    // Operators

    bool
    CppMetaSchema::operator==(const CppMetaSchema& other) const {
        return this->id == other.id && this->name == other.name;
    }

    bool
    CppMetaSchema::operator!=(const CppMetaSchema& other) const {
        return !(*this == other);
    }

    bool
    CppMetaColumn::operator==(const CppMetaColumn& other) const {
        return this->id == other.id && this->name == other.name && this->table_id == other.table_id;
    }

    bool
    CppMetaColumn::operator!=(const CppMetaColumn& other) const {
        return !(*this == other);
    }

    bool
    CppMetaTable::operator==(const CppMetaTable& other) const {
        return this->id == other.id && this->name == other.name && this->schema_id == other.schema_id;
    }

    bool
    CppMetaTable::operator!=(const CppMetaTable& other) const {
        return !(*this == other);
    }

    // Content comparison methods
    bool
    CppMetaSchema::compare_content(const CppMetaSchema& other) const {
        return name == other.name;
    }

    CppMetaSchema::CppMetaSchema(std::string name, std::string db_name) 
        : name(name), db_name(db_name) {
        id = make_uuid_str();
    }

    bool
    CppMetaColumn::compare_content(const CppMetaColumn& other) const {
        return name == other.name && 
               table_id == other.table_id &&
               data_type == other.data_type &&
               flags == other.flags;
    }

    bool
    CppMetaTable::compare_content(const CppMetaTable& other) const {
        if (name != other.name || schema_id != other.schema_id || last_rid != other.last_rid) {
            return false;
        }
        
        if (columns.size() != other.columns.size()) {
            return false;
        }
        
        for (size_t i = 0; i < columns.size(); ++i) {
            if (!columns[i].compare_content(other.columns[i])) {
                return false;
            }
        }
        
        return true;
    }

    // C <=> C++ conversion methods

    CppMetaSchema
    CppMetaSchema::from_c(const MetaSchema& c_schema) {
        CppMetaSchema result;
        result.id = make_uuid_str(c_schema.id);
        result.name = std::string(c_schema.name);
        result.db_name = std::string(c_schema.db_name);
        return result;
    }

    MetaSchema
    CppMetaSchema::to_c() const {
        MetaSchema result;
        parse_uuid_str(id, result.id);
        result.name = make_c_string(name);
        result.db_name = make_c_string(db_name);
        return result;
    }

    void
    CppMetaSchema::cleanup_c(MetaSchema& schema) {
        delete[] schema.name;
        delete[] schema.db_name;
    }

    CppMetaColumn
    CppMetaColumn::from_c(const MetaColumn& c_column) {
        CppMetaColumn result;
        result.id = make_uuid_str(c_column.id);
        result.table_id = make_uuid_str(c_column.table_id);
        result.name = std::string(c_column.name);
        result.data_type = c_column.data_type;
        result.flags = c_column.flags;
        return result;
    }

    MetaColumn
    CppMetaColumn::to_c() const {
        MetaColumn result;
        parse_uuid_str(id, result.id);
        parse_uuid_str(table_id, result.table_id);
        result.name = make_c_string(name);
        result.data_type = data_type;
        result.flags = flags;
        return result;
    }

    void
    CppMetaColumn::cleanup_c(MetaColumn& column) {
        delete[] column.name;
    }

    CppMetaTable
    CppMetaTable::from_c(const MetaTable& c_table) {
        CppMetaTable result;
        result.id = make_uuid_str(c_table.id);
        result.schema_id = make_uuid_str(c_table.schema_id);
        result.name = std::string(c_table.name);
        result.last_rid = c_table.last_rid;
        
        // Convert columns
        result.columns.reserve(c_table.columns_count);
        for (uint64_t i = 0; i < c_table.columns_count; ++i) {
            result.columns.push_back(CppMetaColumn::from_c(c_table.columns[i]));
        }
        
        return result;
    }

    MetaTable
    CppMetaTable::to_c() const {
        MetaTable result;
        parse_uuid_str(id, result.id);
        parse_uuid_str(schema_id, result.schema_id);
        result.name = make_c_string(name);
        result.last_rid = last_rid;
        
        // Convert columns
        result.columns_count = columns.size();
        if (result.columns_count > 0) {
            result.columns = new MetaColumn[result.columns_count];
            for (size_t i = 0; i < result.columns_count; ++i) {
                result.columns[i] = columns[i].to_c();
            }
        } else {
            result.columns = nullptr;
        }
        
        // Find primary key
        result.has_pk = false;
        result.pk = nullptr;
        for (size_t i = 0; i < result.columns_count; ++i) {
            if (result.columns[i].flags & CF_PK) {
                result.has_pk = true;
                result.pk = &result.columns[i];
                break;
            }
        }
        
        return result;
    }

    void
    CppMetaTable::cleanup_c(MetaTable& table) {
        delete[] table.name;
        if (table.columns) {
            for (uint64_t i = 0; i < table.columns_count; ++i) {
                CppMetaColumn::cleanup_c(table.columns[i]);
            }
            delete[] table.columns;
        }
    }

    // Constructors
    CppMetaColumn::CppMetaColumn(std::string name, DataType type, DataColumnFlags flags, std::string table_id)
        : name(std::move(name)), data_type(type), flags(flags), table_id(std::move(table_id)) {
        id = make_uuid_str();
    }
}
