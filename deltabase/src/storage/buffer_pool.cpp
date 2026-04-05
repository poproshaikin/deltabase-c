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
            io_.write_page(page_entry.value, true);
            page_entry.dirty = false;
        }
    }

    void
    BufferPool::flush(IndexFileBuffer::CacheEntry& index_file_entry)
    {
        if (index_file_entry.dirty)
        {
            io_.write_index_file(index_file_entry.value, true);
            index_file_entry.dirty = false;
        }
    }

    void
    BufferPool::initialize()
    {
        data_pages_per_table_ = io_.map_data_pages_for_table();
        index_files_per_table_ = io_.map_index_files_for_table();
    };

    DataPage*
    BufferPool::create_dp(const MetaTable& mt)
    {
        DataPageId id = DataPageId::make();
        DataPage new_page = io_.create_page(mt, id);
        data_pages_.put(id, std::move(new_page), data_page_flusher_);

        auto it = data_pages_per_table_.find(mt.id);
        if (it == data_pages_per_table_.end())
            data_pages_per_table_[mt.id] = { id };
        else
            data_pages_per_table_.at(mt.id).push_back(id);

        return &data_pages_.get(id)->value;
    }

    void
    BufferPool::put_dp(const DataPageId& page_id, DataPage&& page)
    {
        data_pages_.put(page_id, {std::move(page)}, data_page_flusher_);
    }

    DataPage*
    BufferPool::get_dp(const DataPageId& page_id)
    {
        auto* entry = data_pages_.get(page_id);

        if (!entry)
        {
            auto loaded_page = io_.read_data_page(page_id);
            if (!loaded_page)
                return nullptr;

            data_pages_.put(page_id, std::move(*loaded_page), data_page_flusher_);

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
            for (const auto& page_id : data_pages_per_table_[mt.id])
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
        auto pages_list_it = data_pages_per_table_.find(table_id);
        if (pages_list_it == data_pages_per_table_.end())
            return {};

        std::vector<DataPage*> pages;

        for (const auto& page_id : pages_list_it->second)
            pages.push_back(get_dp(page_id));

        return pages;
    }

    IndexFile*
    BufferPool::get_table_index(const Uuid& table_id, const IndexId& index_id)
    {
        auto index_files_list_it = index_files_per_table_.find(table_id);
        if (index_files_list_it == index_files_per_table_.end())
            return nullptr;

        bool found = false;
        for (const auto& index : index_files_list_it->second)
            if (index == index_id)
                found = true;

        if (!found)
            return nullptr;

        auto* entry = index_files_.get(index_id);

        if (!entry)
        {
            auto loaded_file = io_.read_index_file(index_id);
            if (!loaded_file)
                return nullptr;

            index_files_.put(index_id, std::move(*loaded_file), index_file_flusher_);
            entry = index_files_.get(index_id);
        }

        return entry ? &entry->value : nullptr;
    }

    void
    BufferPool::create_table_index(
        const std::string& schema_name, const MetaTable& table, const MetaIndex& index
    )
    {
        IndexFile file = io_.create_index_file(schema_name, table.name, index);

        index_files_.put(index.id, std::move(file), index_file_flusher_);
        index_files_.mark_dirty(file.index_id);

        auto& indexes_list = index_files_per_table_.at(table.id);
        indexes_list.push_back(index.id);
    }

    IndexFile*
    BufferPool::dirty_if(const IndexId& index_id)
    {
        index_files_.mark_dirty(index_id);
        auto* entry = index_files_.get(index_id);
        return entry ? &entry->value : nullptr;
    }

    DataPage*
    BufferPool::dirty_dp(const DataPageId& page_id)
    {
        data_pages_.mark_dirty(page_id);
        auto* entry = data_pages_.get(page_id);
        return entry ? &entry->value : nullptr;
    }

    void
    BufferPool::flush_dirty()
    {
        for (auto& page : data_pages_ | std::views::values)
        {
            flush(page);
        }

        for (auto& index : index_files_ | std::views::values)
        {
            flush(index);
        }
    }

} // namespace storage
