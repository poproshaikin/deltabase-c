#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <memory>

#include "../../sql/include/lexer.hpp"
#include "../../sql/include/parser.hpp"
#include "meta_schema.hpp"

namespace catalog { 

    class MetaRegistry {

        std::unordered_map<std::string, std::shared_ptr<CppMetaSchemaWrapper>> registry;

        void
        init();

      public:
        MetaRegistry();

        std::optional<std::shared_ptr<CppMetaSchemaWrapper>>
        get_schema(const std::string& id) const;


        bool 
        has_table(const std::string& table) const;

        bool
        has_table(const sql::TableIdentifier& identifier) const;

        bool
        has_virtual_table(const sql::TableIdentifier& identifier) const;


        CppMetaTable
        get_table(const std::string& table) const;

        CppMetaTable
        get_table(const sql::SqlToken& table) const;

        CppMetaTable
        get_table(const sql::TableIdentifier& identifier) const;
        
        CppMetaTable
        get_virtual_table(const sql::TableIdentifier& identifier) const;

        std::vector<CppMetaTable>
        get_tables() const;

        std::vector<CppMetaColumn>
        get_columns() const;

        template<typename T>
        void add_schema(T&& schema);
    };

    bool
    is_table_virtual(const sql::TableIdentifier& table);
} // namespace catalog