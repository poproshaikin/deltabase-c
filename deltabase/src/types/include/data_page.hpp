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
    struct DataPageHeader
    {
        Uuid id;
        Uuid table_id;
        RowId min_rid = 0;
        RowId max_rid = 0;
        uint64_t rows_count = 0;
    };

    struct DataPage
    {
        static constexpr uint64_t MAX_SIZE = 1 * 1024 * 1024; // 1 MB

        uint64_t size; // + do not serialize
        fs::path path; // |

        DataPageHeader header;
        std::vector<DataRow> rows;

        DataPage() = default;
    };
}

#endif //DELTABASE_DATA_PAGE_HPP