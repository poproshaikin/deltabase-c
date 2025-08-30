#pragma once

#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <type_traits>

#include "../../sql/include/parser.hpp"
#include "meta_schema.hpp"

extern "C" {
#include "../../core/include/meta.h"
}

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

        template<typename T>
        void add_schema(T&& schema) {
            static_assert(std::is_base_of_v<CppMetaSchemaWrapper, std::decay_t<T>>, 
                          "T must derive from CppMetaSchemaWrapper");
            registry[schema.get_id()] = std::make_unique<std::decay_t<T>>(std::forward<T>(schema));
        }
    };

    MetaColumn
    create_meta_column(const std::string& name, DataType type, DataColumnFlags flags);

    MetaTable
    create_meta_table(const std::string& name, const std::vector<sql::ColumnDefinition> col_defs);

    void
    cleanup_meta_table(MetaTable& table);

    void
    cleanup_meta_column(MetaColumn& column);

    void
    cleanup_meta_table(MetaTable* table);

    void
    cleanup_meta_column(MetaColumn* column);
} // namespace catalog