//
// Created by poproshaikin on 26.11.25.
//

#ifndef DELTABASE_DATA_PAGE_HPP
#define DELTABASE_DATA_PAGE_HPP
#include "UUID.hpp"
#include "data_row.hpp"
#include "page_id.hpp"
#include "typedefs.hpp"

#include <filesystem>

namespace types
{
    using LSN = uint64_t;

    struct DataPage
    {
        static constexpr uint64_t MAX_SIZE = 32 * 1024; // 32 kB
        static constexpr uint64_t HEADER_SIZE =
            sizeof(uuid_t) * 2 + sizeof(RowId) * 2 + sizeof(uint64_t) + sizeof(LSN);

        DataPageId id;
        UUID table_id;
        RowId min_rid = 0;
        RowId max_rid = 0;
        uint64_t rows_count = 0;
        LSN last_lsn = 0;

        uint64_t size; //     |
        fs::path path; //     | do not serialize

        std::vector<DataRow> rows;

        DataPage() = default;

        static DataPage
        make(const fs::path& base_path, const UUID& table_id, const UUID& page_id)
        {
            DataPage page;
            page.id = page_id;
            page.table_id = table_id;
            page.size = HEADER_SIZE;
            page.path = base_path / page.id.to_string();
            return page;
        }
    };
} // namespace types

#endif // DELTABASE_DATA_PAGE_HPP