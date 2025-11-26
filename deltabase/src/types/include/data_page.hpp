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
        RowId min_rid;
        RowId max_rid;
        uint64_t rows_count;
    };

    struct DataPage
    {
        DataPageHeader header;
        std::vector<DataRow> rows;
    };
}

#endif //DELTABASE_DATA_PAGE_HPP