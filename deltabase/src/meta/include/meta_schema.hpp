#pragma once 

#include <string>
#include <vector>

extern "C" {
#include "../../core/include/meta.h"
}

namespace meta {

    struct CppMetaSchemaWrapper
    {
        virtual ~CppMetaSchemaWrapper() = 0;

        virtual std::string
        get_id() const = 0;
    };

    struct CppMetaColumn : public CppMetaSchemaWrapper {

        CppMetaColumn(MetaColumn column) : raw_column(column) {
        }

        std::string
        get_id() const override {
            return std::string(reinterpret_cast<const char*>(raw_column.id));
        }

        std::string
        get_name() const {
            return std::string(reinterpret_cast<const char*>(raw_column.name));
        }
    private:

        MetaColumn raw_column;
    };

    struct CppMetaTable : public CppMetaSchemaWrapper {

        CppMetaTable(MetaTable table) : raw_table(table), columns(parse_columns(table)) {
        }

        std::string
        get_id() const override {
            return std::string(reinterpret_cast<const char*>(raw_table.id));
        }
        
        std::string
        get_name() const {
            return std::string(reinterpret_cast<const char*>(raw_table.name));
        }

        std::vector<CppMetaColumn>
        get_columns() const {
            return this->columns;
        }

    private:

        MetaTable raw_table;
        std::vector<CppMetaColumn> columns;

        std::vector<CppMetaColumn>
        parse_columns(const MetaTable& table) const;
    };

    
}