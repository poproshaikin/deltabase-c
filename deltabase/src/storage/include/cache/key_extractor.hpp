#pragma once

#include <concepts>
#include <string>
#include "../objects/meta_object.hpp"
#include "../pages/page.hpp"

namespace storage {
    template<typename TExtractor, typename TKey, typename TValue>
    concept key_extractor = requires(TExtractor extractor, TValue value) {
        { extractor(value) } -> std::same_as<TKey>;
        { TExtractor::make_key(value) } -> std::same_as<TKey>;
    };

    std::string 
    make_schema_key(std::string schema_name) {
        return schema_name;
    }

    std::string
    make_key(const MetaSchema& schema) {
        return make_schema_key(schema.name);
    }

    std::string
    make_table_key(const std::string& schema_id, const std::string& table_name) {
        return schema_id + "." + table_name;
    }

    std::string
    make_key(const MetaTable& table) {
        return make_table_key(table.schema_id, table.name);
    }

    std::string
    make_page_key(const std::string& page_id) {
        return page_id;
    }

    std::string
    make_key(const DataPage& page) {
        return page.id();
    }
}