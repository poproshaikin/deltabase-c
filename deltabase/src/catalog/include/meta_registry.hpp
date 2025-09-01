#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "../../sql/include/lexer.hpp"
#include "../../sql/include/parser.hpp"
#include "meta_object.hpp"

namespace catalog { 

    class MetaRegistry {

        std::unordered_map<std::string, std::shared_ptr<CppMetaObjectWrapper>> registry_;

        void
        init();

        template<typename T>
        auto
        has_object(const std::string& name) const -> bool;

        template <typename T>
        auto
        get_object(const std::string& name) const -> const T&;

    public:
        MetaRegistry();


        auto
        has_schema(const std::string& name) const -> bool;

        auto
        get_schema(const std::string& name) const -> const CppMetaSchema&;


        auto
        has_table(const std::string& name) const -> bool;

        auto
        has_table(const sql::TableIdentifier& identifier) const -> bool;

        auto
        has_virtual_table(const sql::TableIdentifier& identifier) const -> bool;


        auto
        get_table(const std::string& table) const -> CppMetaTable;

        auto
        get_table(const sql::SqlToken& table) const -> CppMetaTable;

        auto
        get_table(const sql::TableIdentifier& identifier) const -> CppMetaTable;

        auto
        get_virtual_table(const sql::TableIdentifier& identifier) const -> CppMetaTable;

        auto
        get_tables() const -> std::vector<CppMetaTable>;


        auto
        get_columns() const -> std::vector<CppMetaColumn>;

        template <typename T>
        void
        add_schema(T&& schema);
    };

    auto
    is_table_virtual(const sql::TableIdentifier& table) -> bool;
} // namespace catalog