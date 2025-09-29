#pragma once 

#include <concepts>
#include <string>
#include "../pages/page.hpp"
#include "../objects/meta_object.hpp"
#include "../file_manager.hpp"

namespace storage {

    template <typename T, typename K, typename V>
    concept ExternalDataAccessor_c = requires(T accessor, const K& key) {
        { accessor.has(key) } -> std::same_as<bool>;
        { accessor.get(key) } -> std::same_as<V>;
    };

    class data_page_accessor {
        file_manager& fm_;
        std::string db_name_;
    public:
        explicit data_page_accessor(const std::string& db_name, file_manager& fm);
        bool
        has(std::string id) const noexcept;
        data_page
        get(std::string id);
    };

    class meta_table_accessor {
        file_manager& fm_;
        std::string db_name_;
    public:
        explicit meta_table_accessor(const std::string& db_name, file_manager& fm);
        bool
        has(std::string id) const noexcept;
        meta_table
        get(std::string id);
    };

    class meta_schema_accessor {
        file_manager& fm_;
        std::string db_name_;
    public:
        explicit meta_schema_accessor(const std::string& db_name, file_manager& fm);
        bool
        has(std::string id) const noexcept;
        meta_schema
        get(std::string id);
    };

} // namespace storage