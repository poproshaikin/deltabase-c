//
// Created by poproshaikin on 3/28/26.
//

#include "include/buffer_pool.hpp"

#include "../misc/include/utils.hpp"
#include <algorithm>
#include <limits>
#include <mutex>

namespace storage
{
    using namespace types;
    void
    BufferPool::initialize()
    {
        std::lock_guard lock(mutex_);
        initialize_impl();
    }

    void
    BufferPool::initialize_impl()
    {
        data_pages_per_table_ = io_.map_data_pages_for_table();
        index_files_per_table_ = io_.map_index_files_for_table();
    }

    DataPage*
    BufferPool::create_dp_impl(const MetaTable& mt)
    {
        DataPageId id = DataPageId::make();
        DataPage new_page = io_.create_page(mt, id);
        data_pages_.put(id, std::move(new_page), data_page_flusher_);

        auto it = data_pages_per_table_.find(mt.id);
        if (it == data_pages_per_table_.end())
            data_pages_per_table_[mt.id] = {id};
        else
            data_pages_per_table_.at(mt.id).push_back(id);

        return &data_pages_.get(id)->value;
    }

    void
    BufferPool::put_dp(const DataPageId& page_id, DataPage&& page)
    {
        std::lock_guard lock(mutex_);
        put_dp_impl(page_id, std::move(page));
    }

    void
    BufferPool::put_dp_impl(const DataPageId& page_id, DataPage&& page)
    {
        data_pages_.put(page_id, {std::move(page)}, data_page_flusher_);
    }

    DataPage*
    BufferPool::get_dp(const DataPageId& page_id)
    {
        std::lock_guard lock(mutex_);
        return get_dp_impl(page_id);
    }

    DataPage*
    BufferPool::get_dp_impl(const DataPageId& page_id)
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
        std::lock_guard lock(mutex_);
        return prepare_dp_impl(size, mt);
    }

    DataPage*
    BufferPool::prepare_dp_impl(size_t size, const MetaTable& mt)
    {
        auto table_pages_it = data_pages_per_table_.find(mt.id);
        if (table_pages_it != data_pages_per_table_.end())
        {
            for (const auto& page_id : table_pages_it->second)
            {
                auto* page = get_dp_impl(page_id);
                if (!page)
                    continue;

                if (page->size + size <= DataPage::MAX_SIZE)
                    return page;
            }
        }

        auto* new_page = create_dp_impl(mt);

        DataPage* tail_page = nullptr;
        if (table_pages_it != data_pages_per_table_.end())
        {
            for (const auto& page_id : table_pages_it->second)
            {
                auto* existing_page = get_dp_impl(page_id);
                if (!existing_page)
                    continue;

                if (existing_page->next != DataPageId::null())
                    continue;

                if (!tail_page || existing_page->max_rid > tail_page->max_rid)
                    tail_page = existing_page;
            }
        }

        if (tail_page)
        {
            tail_page->next = new_page->id;
            dirty_dp_impl(tail_page->id);
        }

        return new_page;
    }

    void
    BufferPool::append_row(DataPage* destination, MetaTable& mt, const DataRow& new_row, LSN lsn)
    {
        std::lock_guard lock(mutex_);
        append_row_impl(destination, mt, new_row, lsn);
    }

    void
    BufferPool::append_row_impl(DataPage* destination, MetaTable& mt, const DataRow& new_row, LSN lsn)
    {
        destination->rows.push_back(new_row);
        destination->rows_count = destination->rows.size();
        destination->size += io_.estimate_size(new_row);
        destination->min_rid =
            destination->rows_count == 1
                ? new_row.id
                : std::min(destination->min_rid, new_row.id);
        destination->max_rid = std::max(destination->max_rid, new_row.id);
        mt.total_rows++;
        destination->last_lsn = lsn;
        dirty_dp_impl(destination->id);
    }

    std::vector<DataPage*>
    BufferPool::get_table_data(const TableId& table_id)
    {
        std::lock_guard lock(mutex_);
        return get_table_data_impl(table_id);
    }

    std::vector<DataPage*>
    BufferPool::get_table_data_impl(const TableId& table_id)
    {
        auto pages_list_it = data_pages_per_table_.find(table_id);
        if (pages_list_it == data_pages_per_table_.end())
            return {};

        std::vector<DataPage*> pages;

        for (const auto& page_id : pages_list_it->second)
            pages.push_back(get_dp_impl(page_id));

        return pages;
    }

    IndexFile*
    BufferPool::get_table_index(const UUID& table_id, const IndexId& index_id)
    {
        std::lock_guard lock(mutex_);
        return get_table_index_impl(table_id, index_id);
    }

    IndexFile*
    BufferPool::get_table_index_impl(const UUID& table_id, const IndexId& index_id)
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
        const std::string& schema_name,
        const MetaTable& table,
        const MetaIndex& index,
        LSN last_lsn
    )
    {
        std::lock_guard lock(mutex_);
        create_table_index_impl(schema_name, table, index, last_lsn);
    }

    void
    BufferPool::create_table_index_impl(
        const std::string& schema_name,
        const MetaTable& table,
        const MetaIndex& index,
        LSN last_lsn
    )
    {
        IndexFile file = io_.create_index_file(schema_name, table.name, index);
        file.last_lsn = last_lsn;

        index_files_.put(index.id, std::move(file), index_file_flusher_);
        index_files_.mark_dirty(file.index_id);

        auto it = index_files_per_table_.find(table.id);
        if (it == index_files_per_table_.end())
            index_files_per_table_[table.id] = {index.id};
        else
            it->second.push_back(index.id);
    }

    IndexFile*
    BufferPool::dirty_if(const IndexId& index_id)
    {
        std::lock_guard lock(mutex_);
        return dirty_if_impl(index_id);
    }

    IndexFile*
    BufferPool::dirty_if_impl(const IndexId& index_id)
    {
        index_files_.mark_dirty(index_id);
        auto* entry = index_files_.get(index_id);
        return entry ? &entry->value : nullptr;
    }

    void
    BufferPool::set_if_lsn(const IndexId& index_id, LSN last_lsn)
    {
        std::lock_guard lock(mutex_);
        set_if_lsn_impl(index_id, last_lsn);
    }

    void
    BufferPool::set_if_lsn_impl(const IndexId& index_id, LSN last_lsn)
    {
        auto* entry = index_files_.get(index_id);
        if (!entry)
            return;

        entry->value.last_lsn = std::max(entry->value.last_lsn, last_lsn);
    }

    DataPage*
    BufferPool::mark_dirty_impl(const DataPageId& page_id)
    {
        data_pages_.mark_dirty(page_id);
        auto* entry = data_pages_.get(page_id);
        return entry ? &entry->value : nullptr;
    }

    DataPage*
    BufferPool::dirty_dp(const DataPageId& page_id)
    {
        std::lock_guard lock(mutex_);
        return dirty_dp_impl(page_id);
    }

    DataPage*
    BufferPool::dirty_dp_impl(const DataPageId& page_id)
    {
        return mark_dirty_impl(page_id);
    }

    void
    BufferPool::flush_dirty()
    {
        flush_dirty_impl();
    }

    void
    BufferPool::flush_dirty_impl()
    {
        flush_dirty_impl(std::numeric_limits<LSN>::max());
    }

    void
    BufferPool::flush_dirty(LSN max_lsn)
    {
        flush_dirty_impl(max_lsn);
    }

    void
    BufferPool::flush_dirty_impl(LSN max_lsn)
    {
        auto flush_buffer = [this, max_lsn]<typename TKey, typename TValue>(
            misc::Cache<TKey, TValue, cache::LRUPolicy<TKey>>& buffer)
        {
            std::vector<std::pair<TKey, TValue>> to_write;
            {
                std::lock_guard lock(mutex_);
                for (auto& [id, entry] : buffer)
                {
                    if (!entry.dirty || entry.value.last_lsn > max_lsn)
                        continue;
                    to_write.emplace_back(id, entry.value);
                }
            }

            for (const auto& [id, value] : to_write)
                io_.write(value, true);

            {
                std::lock_guard lock(mutex_);
                for (const auto& [id, value] : to_write)
                {
                    auto* entry = buffer.get(id);
                    if (entry && entry->value.last_lsn <= max_lsn)
                        entry->dirty = false;
                }
            }
        };

        flush_buffer(data_pages_);
        flush_buffer(index_files_);
    }

    void
    BufferPool::flush(DataPageBuffer::CacheEntry& page_entry)
    {
        bool is_dirty;
        {
            std::lock_guard lock(mutex_);
            is_dirty = page_entry.dirty;
        }

        if (is_dirty)
        {
            io_.write(page_entry.value, true);
            std::lock_guard lock(mutex_);
            page_entry.dirty = false;
        }
    }

    void
    BufferPool::flush(IndexFileBuffer::CacheEntry& index_file_entry)
    {
        bool is_dirty;
        {
            std::lock_guard lock(mutex_);
            is_dirty = index_file_entry.dirty;
        }

        if (is_dirty)
        {
            io_.write(index_file_entry.value, true);
            std::lock_guard lock(mutex_);
            index_file_entry.dirty = false;
        }
    }


    bool
    BufferPool::is_row_obsolete(const RowPtr& row_ptr)
    {
        std::lock_guard lock(mutex_);
        return is_row_obsolete_impl(row_ptr);
    }

    bool
    BufferPool::is_row_obsolete_impl(const RowPtr& row_ptr)
    {
        const auto* page = get_dp_impl(row_ptr.first);
        if (!page)
            return false;

        for (const auto& row : page->rows)
            if (row.id == row_ptr.second)
                return has_flag(row.flags, DataRowFlags::OBSOLETE);

        return false;
    }

} // namespace storage