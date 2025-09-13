#include "include/pages/page.hpp"
#include "include/objects/data_object.hpp"
#include "include/objects/meta_object.hpp"

#include <format>

namespace storage {
    std::string
    DataPage::id() const {
        return id_;
    }

    uint64_t
    DataPage::size() const {
        return size_;
    }
    
    RowId
    DataPage::min_rid() const {
        return min_rid_;
    }

    RowId
    DataPage::max_rid() const {
        return max_rid_;
    }

    void
    DataPage::mark_dirty() {
        is_dirty_ = true;
    }

    const std::vector<DataRow>&
    DataPage::rows() const {
        return rows_;
    }

    bool
    DataPage::has_row(RowId rid) const {
        for (const auto& row : rows_) {
            if (row.row_id == rid) {
                return true;
            }
        }

        return false;
    }

    DataRow&
    DataPage::get_row(RowId rid) {
        for (auto& row : rows_) {
            if (row.row_id == rid) {
                return row;
            }
        }

        throw std::runtime_error(std::format("Row with id {} was not found in DataPage {}", rid, id_));
    }

    RowId
    DataPage::insert_row(MetaTable& table, DataRow& row) {
        row.row_id = ++table.last_rid;
        rows_.push_back(row);
        return row.row_id;
    }

    void
    DataPage::delete_row(RowId id) {
        if (!has_row(id)) {
            throw std::runtime_error("");
        }

        get_row(id).flags |= DataRowFlags::OBSOLETE;
    }

    RowId
    DataPage::update_row(MetaTable& table, RowId old_row_id, DataRowUpdate& update) {
        if (!has_row(old_row_id)) {
            throw std::runtime_error("");
        }

        auto& old_row = get_row(old_row_id);
        old_row.flags |= DataRowFlags::OBSOLETE;

        auto new_row = old_row.update(update);
        insert_row(table, new_row);

        return new_row.row_id;
    }

}