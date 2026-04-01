//
// Created by poproshaikin on 4/1/26.
//

#ifndef DELTABASE_INDEX_BPLUS_TREE_HPP
#define DELTABASE_INDEX_BPLUS_TREE_HPP
#include "index_pager.hpp"

namespace storage
{
    class IndexBPlusTree
    {
        IIndexPager& pager_;
        size_t max_leaf_keys_;
        size_t max_internal_keys_;

        static int
        compare_token(const types::DataToken& l, const types::DataToken& r);

        types::IndexPage*
        root();

        types::IndexPage*
        find_leaf(const types::DataToken& key, std::vector<types::IndexPageId>* path = nullptr);

        void
        insert_into_leaf(types::LeafIndexNode& leaf, const types::DataToken& key, const types::RowPtr& row_ptr);

        void
        split_leaf_and_propagate(types::IndexPage& leaf_page, std::vector<types::IndexPageId>& path);

        void
        insert_into_parent(
            std::vector<types::IndexPageId>& path,
            types::IndexPageId left_id,
            const types::DataToken& separator,
            types::IndexPageId right_id
        );

        void
        split_internal_and_propagate(types::IndexPage& internal_page, std::vector<types::IndexPageId>& path);

    public:
        IndexBPlusTree(IIndexPager& pager, size_t max_leaf_keys = 64, size_t max_internal_keys = 64)
            : pager_(pager), max_leaf_keys_(max_leaf_keys), max_internal_keys_(max_internal_keys)
        {
        }

        std::optional<types::RowPtr>
        find(const types::DataToken& key);

        void
        insert(const types::DataToken& key, const types::RowPtr& row_ptr);
    };
}

#endif // DELTABASE_INDEX_BPLUS_TREE_HPP
