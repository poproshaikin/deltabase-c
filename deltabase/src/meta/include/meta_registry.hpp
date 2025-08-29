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

// сделать что то типа большого реестра мета-сущностей, типа таблиц, колонок, всякой хуйни
// при инициализации движка будет подгружать всю мета информацию.
// schema catalog
// нет обращений к fs при каждом запросе
// единый источник метаданных
// пизда круто

namespace meta {

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
} // namespace meta