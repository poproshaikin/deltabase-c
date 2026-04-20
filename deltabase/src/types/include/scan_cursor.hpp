//
// Created by poproshaikin on 4/20/26.
//

#ifndef DELTABASE_SCAN_CURSOR_HPP
#define DELTABASE_SCAN_CURSOR_HPP
#include "meta_table.hpp"

namespace types
{
    struct ScanCursor
    {
        DataPageId page;
        int slot;
        int chunk_size;

        bool initialized;
    };
}

#endif //DELTABASE_SCAN_CURSOR_HPP