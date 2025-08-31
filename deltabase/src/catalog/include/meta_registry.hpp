#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <memory>
#include <type_traits>

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
        get_schema(const std::string& id) {
            auto it = this->registry.find(id);
            if (it != this->registry.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        std::shared_ptr<CppMetaTable>
        get_table(const std::string& table) {
            for (const auto& kvp : this->registry) {
                if (kvp.second->get_name() == table) {
                    return std::dynamic_pointer_cast<CppMetaTable>(kvp.second);
                }
            }
            return nullptr;
        }

        std::shared_ptr<CppMetaTable>
        get_table(const sql::SqlToken& table) {
            return this->get_table(table.value);
        }

        template<typename T>
        void add_schema(T&& schema) {
            static_assert(std::is_base_of_v<CppMetaSchemaWrapper, std::decay_t<T>>, 
                          "T must derive from CppMetaSchemaWrapper");
            registry[schema.get_id()] = std::make_unique<std::decay_t<T>>(std::forward<T>(schema));
        }
    };


    bool
    is_table_virtual(const sql::TableIdentifier& table);
} // namespace catalog