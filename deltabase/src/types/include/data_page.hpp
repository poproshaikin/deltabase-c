//
// Created by poproshaikin on 26.11.25.
//

#ifndef DELTABASE_DATA_PAGE_HPP
#define DELTABASE_DATA_PAGE_HPP
#include "data_row.hpp"
#include "typedefs.hpp"
#include "uuid.hpp"

namespace types
{
    struct DataPageHeader
    {
        Uuid id;
        Uuid table_id;
        uint64_t size = 0;
        RowId min_rid = 0;
        RowId max_rid = 0;
        uint64_t rows_count = 0;
    };

    struct DataPage
    {
        static inline uint64_t last_version_ = 0;
        static constexpr uint64_t MAX_SIZE = 1 * 1024 * 1024; // 1 MB

        DataPageHeader header;
        uint64_t version;
        std::vector<DataRow> rows;

        DataPage() : version(last_version_++)
        {
        }
    };
}

#endif //DELTABASE_DATA_PAGE_HPP