#include "include/pages/page_buffers.hpp"

namespace storage {
    int
    PageBuffers::insert_row(MetaTable& table, DataRow& row) {
        uint64_t size = row.estimate_size();
        DataPage& page = has_available_page(size) ? get_available_page(size) : create_page();

        page.insert_row(table, row);
        page.mark_dirty();
        tables_.mark_dirty(table.id);

        return 0;
    }
}