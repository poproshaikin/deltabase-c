//
// Created by poproshaikin on 26.11.25.
//

#ifndef DELTABASE_DATA_PAGE_HPP
#define DELTABASE_DATA_PAGE_HPP
#include "data_row.hpp"
#include "typedefs.hpp"
#include "uuid.hpp"

#include <filesystem>

namespace types
{

    using PageId = Uuid;
    using LSN = uint64_t;

    // DTO which is serialized for storing in a page
    struct DataPageHeader
    {
        PageId id;
        Uuid table_id;
        RowId min_rid = 0;
        RowId max_rid = 0;
        uint64_t rows_count = 0;
        LSN last_lsn = 0;

        static constexpr uint64_t SIZE =
            sizeof(uuid_t) * 2 + sizeof(RowId) * 2 + sizeof(uint64_t) + sizeof(LSN);
    };

    struct DataPage
    {
        static constexpr uint64_t MAX_SIZE = 32 * 1024; // 32 kB

        uint64_t size; // + do not serialize
        fs::path path; // |

        DataPageHeader header;
        std::vector<DataRow> rows;

        DataPage() = default;

        static DataPage
        make(const fs::path& base_path, const Uuid& table_id)
        {
            DataPage page;
            page.header.id = Uuid::make();
            page.header.table_id = table_id;
            page.size = DataPageHeader::SIZE;
            page.path = base_path / page.header.id.to_string();
            return page;
        }
    };
} // namespace types

#endif // DELTABASE_DATA_PAGE_HPP