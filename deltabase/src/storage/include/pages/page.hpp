#pragma once 

#include <string>
#include "../objects/data_object.hpp"

namespace storage {
    struct DataPageHeader {
        std::string id;
        uint64_t size;
        RowId min_rid;
        RowId max_rid;
    };

    class DataPage {

        // --- Header ---
        std::string id_;
        uint64_t size_;
        RowId min_rid_;
        RowId max_rid_;

        // ----Props-----
        static const int max_tokens = 50000;
        std::vector<DataRow> rows_;
        bool is_dirty_;

        // ---Methods----
        DataPage(std::string id, uint64_t size, RowId min_rid, RowId max_rid);
        // --------------
        friend class FileManager;

    public:
        static const uint64_t max_size = 16 * 1024;

        DataPage() = default;
        
        // prevent copying (expensive operation with large data)
        DataPage(const DataPage&) = delete;
        DataPage& operator=(const DataPage&) = delete;
        
        // allow moving
        DataPage(DataPage&&) = default;
        DataPage& operator=(DataPage&&) = default;

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

        bool
        can_insert(const DataRow& row) const noexcept;
        RowId
        insert_row(MetaTable& table, DataRow& row);

        RowId
        update_row(MetaTable& table, RowId old_row, DataRowUpdate& update);

        void
        delete_row(RowId id);

        bytes_v
        serialize() const;

        static bool
        try_deserialize(const bytes_v& bytes, DataPage& out_result);
    };
}