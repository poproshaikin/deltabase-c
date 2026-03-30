//
// Created by poproshaikin on 3/28/26.
//

#include "include/buffer_pool.hpp"

#include <ranges>

namespace storage
{
    using namespace types;

    void
    BufferPool::flush(DataPageBuffer::CacheEntry& page_entry)
    {
        if (page_entry.dirty)
        {
            io_.write_page(page_entry.value);
            page_entry.dirty = false;
        }
    }

    void
    BufferPool::initialize()
    {
        pages_to_tables_ = io_.map_tables_pages();
    };

    DataPage*
    BufferPool::create_dp(const MetaTable& mt)
    {
        PageId id = PageId::make();
        DataPage new_page = io_.create_page(mt, id);
        data_pages_.put(id, std::move(new_page), flusher_);

        auto it = pages_to_tables_.find(mt.id);
        if (it == pages_to_tables_.end())
            pages_to_tables_[mt.id] = { id };
        else
            pages_to_tables_.at(mt.id).push_back(id);

        return &data_pages_.get(id)->value;
    }

    void
    BufferPool::put_dp(const PageId& page_id, DataPage&& page)
    {
        data_pages_.put(page_id, {std::move(page)}, flusher_);
    }

    DataPage*
    BufferPool::get_dp(const PageId& page_id)
    {
        auto* entry = data_pages_.get(page_id);

        if (!entry)
        {
            auto loaded_page = io_.read_data_page(page_id);
            if (!loaded_page)
                return nullptr;

            data_pages_.put(page_id, std::move(*loaded_page), flusher_);

            entry = data_pages_.get(page_id);
        }

        return entry ? &entry->value : nullptr;
    }

    DataPage*
    BufferPool::prepare_dp(size_t size, const MetaTable& mt)
    {
        DataPage* result = nullptr;
        for (auto& entry : data_pages_ | std::views::values)
        {
            if (entry.value.size + size > DataPage::MAX_SIZE)
                continue;

            result = &entry.value;
            break;
        }

        if (!result)
        {
            bool found_available = false;
            for (const auto& page_id : pages_to_tables_[mt.id])
            {
                auto* page = get_dp(page_id);
                if (page->size + size > DataPage::MAX_SIZE)
                    continue;

                result = page;
                found_available = true;
                break;
            }

            if (!found_available)
            {
                result = create_dp(mt);
            }
        }

        return result;
    }

    std::vector<DataPage*>
    BufferPool::get_table_data(const TableId& table_id)
    {
        auto pages_list_it = pages_to_tables_.find(table_id);
        if (pages_list_it == pages_to_tables_.end())
            return {};

        std::vector<DataPage*> pages;

        for (const auto& page_id : pages_list_it->second)
        {
            pages.push_back(get_dp(page_id));
        }

        return pages;
    }

    DataPage*
    BufferPool::dirty_dp(const PageId& page_id)
    {
        data_pages_.mark_dirty(page_id);
        auto* entry = data_pages_.get(page_id);
        if (!entry)
            return nullptr;
        return &entry->value;
    }

    void
    BufferPool::flush_dirty()
    {
        for (auto& page : data_pages_ | std::views::values)
        {
            if (page.dirty)
            {
                io_.write_page(page.value, true);
                page.dirty = false;
            }
        }
    }

} // namespace storage
