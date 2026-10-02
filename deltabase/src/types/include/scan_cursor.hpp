//
// Created by poproshaikin on 4/20/26.
//

#ifndef DELTABASE_SCAN_CURSOR_HPP
#define DELTABASE_SCAN_CURSOR_HPP
#include "meta_table.hpp"

#include <vector>

namespace types
{
    struct ScanCursor
    {
        std::vector<DataPageId> pages;
        size_t page_idx = 0;
        size_t row_idx = 0;

        bool initialized;
    };
}

#endif //DELTABASE_SCAN_CURSOR_HPP