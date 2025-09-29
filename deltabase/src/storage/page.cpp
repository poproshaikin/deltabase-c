#include "include/pages/page.hpp"
#include "include/objects/data_object.hpp"
#include "include/objects/meta_object.hpp"

#include <cstring>
#include <format>

namespace storage {
    std::string
    data_page::id() const {
        return id_;
    }

    uint64_t
    data_page::size() const {
        return size_;
    }
    
    RowId
    data_page::min_rid() const {
        return min_rid_;
    }

    RowId
    data_page::max_rid() const {
        return max_rid_;
    }

    void
    data_page::mark_dirty() {
        is_dirty_ = true;
    }

    const std::vector<data_row>&
    data_page::rows() const {
        return rows_;
    }

    bool
    data_page::has_row(RowId rid) const {
        for (const auto& row : rows_) {
            if (row.row_id == rid) {
                return true;
            }
        }

        return false;
    }

    data_row&
    data_page::get_row(RowId rid) {
        for (auto& row : rows_) {
            if (row.row_id == rid) {
                return row;
            }
        }

        throw std::runtime_error(std::format("Row with id {} was not found in DataPage {}", rid, id_));
    }

    bool
    data_page::can_insert(const data_row& row) const noexcept {
        return size_ + row.estimate_size() <= max_size;
    }

    RowId
    data_page::insert_row(meta_table& table, data_row& row) {
        if (!can_insert(row)) {
            throw std::runtime_error(std::format("Page {} hasn't enough space for a new row", id_));
        }

        row.row_id = ++table.last_rid;
        rows_.push_back(row);
        size_ += row.estimate_size();

        return row.row_id;
    }

    void
    data_page::delete_row(RowId id) {
        if (!has_row(id)) {
            throw std::runtime_error("");
        }

        get_row(id).flags |= data_row_flags::OBSOLETE;
    }

    RowId
    data_page::update_row(meta_table& table, RowId old_row_id, data_row_update& update) {
        if (!has_row(old_row_id)) {
            throw std::runtime_error("");
        }

        auto& old_row = get_row(old_row_id);
        old_row.flags |= data_row_flags::OBSOLETE;

        auto new_row = old_row.update(update);
        insert_row(table, new_row);

        return new_row.row_id;
    }

    bytes_v
    data_page::serialize() const {
        bytes_v v;
        uint64_t header_size = strlen(id_.c_str()) + sizeof(size_) + sizeof(min_rid_) + sizeof(max_rid_);
        v.reserve(header_size);

        v.insert(v.end(), id_.begin(), id_.end());
        v.insert(v.end(), &size_, &size_ + sizeof(size_));
        v.insert(v.end(), &min_rid_, &min_rid_ + sizeof(min_rid_));
        v.insert(v.end(), &max_rid_, &max_rid_ + sizeof(max_rid_));

        uint64_t rows_size = 0;
        for (const auto& row : rows_) {
            rows_size += row.estimate_size();
        }

        v.reserve(rows_size);

        for (const auto& row : rows_) {
            auto serialized_row = row.serialize();
            v.insert(v.end(), serialized_row.begin(), serialized_row.end());
        }

        return v;
    }
}