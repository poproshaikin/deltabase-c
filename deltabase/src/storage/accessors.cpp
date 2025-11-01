#include "include/cache/accessors.hpp"
#include "include/file_manager.hpp"
#include "include/objects/meta_object.hpp"
#include "include/pages/page.hpp"
#include <endian.h>

namespace storage {
    DataPageAccessor::DataPageAccessor(const std::string& db_name, FileManager& fm)
        : db_name_(db_name), fm_(fm) {
    }
    
    bool
    DataPageAccessor::has(std::string id) const noexcept {
        return fm_.page_exists(db_name_, id);
    }

    DataPage
    DataPageAccessor::get(std::string id) {
        if (!fm_.page_exists(db_name_, id)) {
            throw std::runtime_error("DataPageAccessor::get: failed to get page by id " + id);
        }

        return fm_.load_page(db_name_, id);
    }

    MetaTableAccessor::MetaTableAccessor(const std::string& db_name, FileManager& fm)
        : fm_(fm), db_name_(db_name) {
    }

    bool
    MetaTableAccessor::has(std::string id) const noexcept {
        return fm_.table_exists(db_name_, id);
    }

    MetaTable
    MetaTableAccessor::get(std::string id) {
        return fm_.load_table(db_name_, id);
    }

    MetaSchemaAccessor::MetaSchemaAccessor(const std::string& db_name, FileManager& fm) 
        : db_name_(db_name), fm_(fm) {
    }

    bool
    MetaSchemaAccessor::has(const std::string id) const noexcept {
        return fm_.schema_exists_by_id(db_name_, id);
    }

    MetaSchema
    MetaSchemaAccessor::get(const std::string id) {
        return fm_.load_schema_by_name(db_name_, id);
    }
}