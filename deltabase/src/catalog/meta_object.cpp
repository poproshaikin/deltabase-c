#include "include/meta_object.hpp"

namespace catalog {
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
}
