//
// Created by poproshaikin on 3/31/26.
//

#ifndef DELTABASE_INDEX_PAGER_HPP
#define DELTABASE_INDEX_PAGER_HPP
#include "index_page.hpp"
#include "meta_index.hpp"

namespace storage
{
    class IIndexPager
    {
    public:
        virtual ~IIndexPager() = default;

        virtual const types::IndexId&
        index_id() const = 0;

        virtual types::IndexPageId
        root_page_id() const = 0;

        virtual void
        set_root_page_id(types::IndexPageId root) = 0;

        virtual types::IndexPage*
        get_page(types::IndexPageId page_id) = 0;

        virtual types::IndexPage*
        create_page(bool is_leaf, types::IndexPageId parent) = 0;

        virtual void
        mark_dirty() = 0;

        virtual void
        flush() = 0;
    };
} // namespace storage

#endif // DELTABASE_INDEX_PAGER_HPP
