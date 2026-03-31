//
// Created by poproshaikin on 3/30/26.
//

#ifndef DELTABASE_INDEX_PAGE_HPP
#define DELTABASE_INDEX_PAGE_HPP

#include "data_token.hpp"
#include "meta_index.hpp"

namespace types
{
    using IndexPageId = uint64_t;
    using RowPtr = std::pair<DataPageId, RowId>;

    constexpr uint64_t MAX_IP_SIZE = 4 * 1024;

    struct InternalIndexNode
    {
        std::vector<DataToken> keys;
        std::vector<IndexPageId> children;
    };

    struct LeafIndexNode
    {
        std::vector<DataToken> keys;
        std::vector<RowPtr> rows;
        IndexPageId next_leaf = 0;
    };

    struct IndexPage
    {
        IndexPageId id;
        IndexPageId parent;
        IndexId index_id;
        bool is_leaf;

        std::variant<InternalIndexNode, LeafIndexNode> data;
    };
} // namespace types

#endif // DELTABASE_INDEX_PAGE_HPP
