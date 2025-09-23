#pragma once 

#include <concepts>
#include <string>
#include "../pages/page.hpp"
#include "../objects/meta_object.hpp"
#include "../file_manager.hpp"

namespace storage {

    template <typename T, typename K, typename V>
    concept ExternalDataAccessor = requires(T accessor, const K& key) {
        { accessor.has(key) } -> std::same_as<bool>;
        { accessor.get(key) } -> std::same_as<V>;
    };

    class DataPageAccessor {
        FileManager& fm_;
        std::string db_name_;
    public:
        explicit DataPageAccessor(const std::string& db_name, FileManager& fm);
        bool
        has(std::string id) const noexcept;
        DataPage
        get(std::string id);
    };

    class MetaTableAccessor {
        FileManager& fm_;
        std::string db_name_;
    public:
        explicit MetaTableAccessor(const std::string& db_name, FileManager& fm);
        bool
        has(std::string id) const noexcept;
        MetaTable
        get(std::string id);
    };

    class MetaSchemaAccessor {
        FileManager& fm_;
        std::string db_name_;
    public:
        explicit MetaSchemaAccessor(const std::string& db_name, FileManager& fm);
        bool
        has(std::string id) const noexcept;
        MetaSchema
        get(std::string id);
    };

} // namespace storage