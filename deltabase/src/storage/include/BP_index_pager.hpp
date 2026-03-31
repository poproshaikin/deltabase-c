//
// Created by poproshaikin on 3/31/26.
//

#ifndef DELTABASE_BP_INDEX_PAGER_HPP
#define DELTABASE_BP_INDEX_PAGER_HPP
#include "buffer_pool.hpp"
#include "index_pager.hpp"

namespace storage
{
    class BPIndexPager : public IIndexPager
    {
        BufferPool& buffer_pool_;
        types::TableId table_id_;
        types::IndexId index_id_;

        types::IndexFile*
        file_or_throw() const;

    public:
        BPIndexPager(
            BufferPool& buffer_pool, const types::TableId& table_id, const types::IndexId& index_id
        );

        const types::IndexId&
        index_id() const override;

        types::IndexPageId
        root_page_id() const override;

        void
        set_root_page_id(types::IndexPageId root) override;

        types::IndexPage*
        get_page(types::IndexPageId page_id) override;

        types::IndexPage*
        create_page(bool is_leaf, types::IndexPageId parent) override;

        void
        mark_dirty() override;

        void
        flush() override;
    };
} // namespace storage

#endif // DELTABASE_BP_INDEX_PAGER_HPP
