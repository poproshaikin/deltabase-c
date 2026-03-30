//
// Created by poproshaikin on 3/30/26.
//

#ifndef DELTABASE_INDEX_PAGE_HPP
#define DELTABASE_INDEX_PAGE_HPP
#include "data_page.hpp"

namespace types
{
    struct IndexEntry
    {
        Bytes key;
        std::variant<RowId, PageId> ptr;
    };

    struct IndexPage
    {
        PageId id;
        PageId parent_page_id;
        PageId next_leaf_page_id;
        bool is_leaf;

        static constexpr auto MAX_ENTRIES_PER_PAGE = 1024;

        std::vector<IndexEntry> entries;
    };
}

#endif // DELTABASE_INDEX_PAGE_HPP
