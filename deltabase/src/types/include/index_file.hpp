//
// Created by poproshaikin on 3/31/26.
//

#ifndef DELTABASE_INDEX_FILE_HPP
#define DELTABASE_INDEX_FILE_HPP
#include "index_page.hpp"
#include "meta_index.hpp"

namespace types
{
    struct IndexFile
    {
        IndexId index_id;
        IndexPageId root_page = 0;
        IndexPageId last_page = 0;
        LSN last_lsn = 0;
        std::vector<IndexPage> pages;
    };
}

#endif // DELTABASE_INDEX_FILE_HPP
