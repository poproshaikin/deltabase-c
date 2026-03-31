//
// Created by poproshaikin on 3/31/26.
//

#include "BP_index_pager.hpp"
namespace storage
{
    using namespace types;

    BPIndexPager::BPIndexPager(
        BufferPool& buffer_pool, const TableId& table_id, const types::IndexId& index_id
    )
        : buffer_pool_(buffer_pool), table_id_(table_id), index_id_(index_id)
    {
    }

    IndexFile*
    BPIndexPager::file_or_throw() const
    {
        auto* file = buffer_pool_.get_table_index(table_id_, index_id_);
        if (!file)
            throw std::runtime_error("BPIndexPager::file_or_throw");
        return file;
    }

    const IndexId&
    BPIndexPager::index_id() const
    {
        return index_id_;
    }

    IndexPageId
    BPIndexPager::root_page_id() const
    {
        auto* file = file_or_throw();
        return file->root_page;
    }

    void
    BPIndexPager::set_root_page_id(IndexPageId root)
    {
        auto* file = buffer_pool_.dirty_if(index_id_);
        if (!file)
            throw std::runtime_error("BPIndexPager::set_root_page_id");
        file->root_page = root;
    }

    IndexPage*
    BPIndexPager::get_page(IndexPageId page_id)
    {
        auto* file = file_or_throw();
        for (auto& p : file->pages)
            if (p.id == page_id)
                return &p;

        return nullptr;
    }

    IndexPage*
    BPIndexPager::create_page(bool is_leaf, IndexPageId parent)
    {
        auto* file = buffer_pool_.dirty_if(index_id_);
        if (!file)
            throw std::runtime_error("BPIndexPager::create_page");

        IndexPage page;
        page.id = ++file->last_page;
        page.is_leaf = is_leaf;
        page.parent = parent;
        page.index_id = index_id_;
        if (is_leaf)
            page.data = LeafIndexNode{};
        else
            page.data = InternalIndexNode{};

        file->pages.push_back(std::move(page));
        return &file->pages.back();
    }

    void
    BPIndexPager::mark_dirty()
    {
        auto* file = buffer_pool_.dirty_if(index_id_);
        if (!file)
            throw std::runtime_error("BPIndexPager::mark_dirty");
    }

    void
    BPIndexPager::flush()
    {
        buffer_pool_.flush_dirty();
    }
} // namespace storage
