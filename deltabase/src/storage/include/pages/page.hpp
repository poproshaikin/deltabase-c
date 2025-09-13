#pragma once 

#include <string>
#include "../objects/data_object.hpp"

namespace storage {
    class DataPage {
        std::string id_;
        uint64_t size_;
        RowId min_rid_;
        RowId max_rid_;
        
        std::vector<DataRow> rows_;
        bool is_dirty_;

    public:
        static constexpr uint64_t MAX_SIZE = 8 * 1024;

        DataPage(const MetaTable& table, bytes_arr bytes);

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
        DataRow&
        get_row(RowId rid);

        const std::vector<DataRow>& 
        rows() const;

        RowId
        insert_row(MetaTable& table, DataRow& row);

        RowId
        update_row(MetaTable& table, RowId old_row, DataRowUpdate& update);

        void
        delete_row(RowId id);
    };
}