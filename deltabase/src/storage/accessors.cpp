#include "include/cache/accessors.hpp"
#include "include/file_manager.hpp"
#include "include/objects/meta_object.hpp"
#include "include/pages/page.hpp"
#include <endian.h>

namespace storage {
    data_page_accessor::data_page_accessor(const std::string& db_name, file_manager& fm)
        : db_name_(db_name), fm_(fm) {
    }
    
    bool
    data_page_accessor::has(std::string id) const noexcept {
        return fm_.page_exists(db_name_, id);
    }

    data_page
    data_page_accessor::get(std::string id) {
        if (!fm_.page_exists(db_name_, id)) {
            throw std::runtime_error("DataPageAccessor::get: failed to get page by id " + id);
        }

        return fm_.load_page(db_name_, id);
    }

    meta_table_accessor::meta_table_accessor(const std::string& db_name, file_manager& fm)
        : fm_(fm), db_name_(db_name) {
    }

    bool
    meta_table_accessor::has(std::string id) const noexcept {
        return fm_.table_exists(db_name_, id);
    }

    meta_table
    meta_table_accessor::get(std::string id) {
        return fm_.load_table(db_name_, id);
    }

    meta_schema_accessor::meta_schema_accessor(const std::string& db_name, file_manager& fm) 
        : db_name_(db_name), fm_(fm) {
    }

    bool
    meta_schema_accessor::has(const std::string id) const noexcept {
        return fm_.schema_exists_by_id(db_name_, id);
    }

    meta_schema
    meta_schema_accessor::get(const std::string id) {
        return fm_.load_schema_by_name(db_name_, id);
    }
}