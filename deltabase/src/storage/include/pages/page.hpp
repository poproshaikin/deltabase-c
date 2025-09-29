#pragma once 

#include <string>
#include "../objects/data_object.hpp"

namespace storage {
    class data_page {
        // --- Header ---
        std::string id_;
        uint64_t size_;
        RowId min_rid_;
        RowId max_rid_;
        // --------------
        
        std::vector<data_row> rows_;
        bool is_dirty_;

    public:
        static constexpr uint64_t max_size = 8 * 1024;

        static bool
        can_deserialize(const bytes_v& bytes);
        static data_page
        deserialize(const bytes_v& bytes);

        data_page() = default;
        data_page(const meta_table& table, bytes_v bytes);

        std::string
        id() const;
        uint64_t
        size() const;
        RowId
        min_rid() const;
        RowId
        max_rid() const;
        void
        mark_dirty();

        bool
        has_row(RowId rid) const;
        data_row&
        get_row(RowId rid);

        const std::vector<data_row>& 
        rows() const;

        bool
        can_insert(const data_row& row) const noexcept;
        RowId
        insert_row(meta_table& table, data_row& row);

        RowId
        update_row(meta_table& table, RowId old_row, data_row_update& update);

        void
        delete_row(RowId id);

        bytes_v serialize() const;
    };
}