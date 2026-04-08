//
// Created by poproshaikin on 3/30/26.
//

#ifndef DELTABASE_META_INDEX_HPP
#define DELTABASE_META_INDEX_HPP
#include "UUID.hpp"
#include "meta_column.hpp"
#include "page_id.hpp"
#include "table_id.hpp"

#include <string>

namespace types
{
    using IndexId = UUID;

    struct MetaIndex
    {
        IndexId id;
        TableId table_id;
        ColumnId column_id;
        DataPageId root_page_id;
        std::string name;
        DataType key_type;
        bool is_unique;
    };
}

#endif // DELTABASE_META_INDEX_HPP
