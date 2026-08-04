//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_BUFFER_POOL_HPP
#define DELTABASE_BUFFER_POOL_HPP

#include "../../misc/include/LRU_policy.hpp"
#include "../../misc/include/cache.hpp"
#include "../../types/include/data_page.hpp"
#include "../../types/include/index_file.hpp"
#include "io_manager.hpp"
#include "../../types/include/UUID.hpp"

namespace storage
{
    template <typename TKey, typename TValue>
    using Buffer = misc::Cache<TKey, TValue, cache::LRUPolicy<TKey>>;
    using DataPageBuffer = Buffer<types::DataPageId, types::DataPage>;
    using IndexFileBuffer = Buffer<types::IndexId, types::IndexFile>;

    class BufferPool
    {

    public:
        BufferPool(IIOManager& io)
            : data_pages_(cache::LRUPolicy<types::DataPageId>{}),
              index_files_(cache::LRUPolicy<types::IndexId>{}),
              io_(io)
        {
        }

        ~BufferPool()
        {
            flush_dirty();
        }

        void
        initialize();

        void
        put_dp(const types::DataPageId& page_id, types::DataPage&& page);

        types::DataPage*
        get_dp(const types::DataPageId& page_id);

        types::DataPage*
        prepare_dp(size_t size, const types::MetaTable& mt);

        void
        append_row(
            types::DataPage* destination,
            types::MetaTable& mt,
            const types::DataRow& new_row,
            types::LSN lsn);

        std::vector<types::DataPage*>
        get_table_data(const types::UUID& table_id);

        types::DataPage*
        dirty_dp(const types::DataPageId& page_id);

        types::IndexFile*
        get_table_index(const types::UUID& table_id, const types::IndexId& index_id);

        void
        create_table_index(
            const std::string& schema_name,
            const types::MetaTable& table,
            const types::MetaIndex& index,
            types::LSN last_lsn
        );

        types::IndexFile*
        dirty_if(const types::IndexId& index_id);

        void
        set_if_lsn(const types::IndexId& index_id, types::LSN last_lsn);

        bool
        is_row_obsolete(const types::RowPtr& row_ptr);

        void
        flush_dirty();
        void
        flush_dirty(types::LSN max_lsn);

    private:

        DataPageBuffer data_pages_;
        IndexFileBuffer index_files_;

        IIOManager& io_;

        std::mutex mutex_;

        std::unordered_map<types::TableId, std::vector<types::DataPageId>> data_pages_per_table_;
        std::unordered_map<types::TableId, std::vector<types::IndexId>> index_files_per_table_;

        void
        flush(DataPageBuffer::CacheEntry& page_entry);
        void
        flush(IndexFileBuffer::CacheEntry& index_file_entry);

        std::function<void(DataPageBuffer::CacheEntry&)> data_page_flusher_ =
            [this](DataPageBuffer::CacheEntry& page_entry) { flush(page_entry); };
        std::function<void(IndexFileBuffer::CacheEntry&)> index_file_flusher_ =
            [this](IndexFileBuffer::CacheEntry& index_file_entry) { flush(index_file_entry); };

        // All *_impl methods assume the caller already holds whatever lock guards
        // data_pages_/index_files_/data_pages_per_table_/index_files_per_table_.
        // They must only call other *_impl methods internally, never the public
        // (locking) API below -- calling a locking public method from here would
        // re-enter the same (non-recursive) lock on the same thread and deadlock.

        void
        initialize_impl();

        void
        put_dp_impl(const types::DataPageId& page_id, types::DataPage&& page);

        types::DataPage*
        get_dp_impl(const types::DataPageId& page_id);

        types::DataPage*
        prepare_dp_impl(size_t size, const types::MetaTable& mt);

        void
        append_row_impl(
            types::DataPage* destination,
            types::MetaTable& mt,
            const types::DataRow& new_row,
            types::LSN lsn);

        std::vector<types::DataPage*>
        get_table_data_impl(const types::UUID& table_id);

        types::DataPage*
        dirty_dp_impl(const types::DataPageId& page_id);

        types::IndexFile*
        get_table_index_impl(const types::UUID& table_id, const types::IndexId& index_id);

        void
        create_table_index_impl(
            const std::string& schema_name,
            const types::MetaTable& table,
            const types::MetaIndex& index,
            types::LSN last_lsn
        );

        types::IndexFile*
        dirty_if_impl(const types::IndexId& index_id);

        void
        set_if_lsn_impl(const types::IndexId& index_id, types::LSN last_lsn);

        bool
        is_row_obsolete_impl(const types::RowPtr& row_ptr);

        void
        flush_dirty_impl();
        void
        flush_dirty_impl(types::LSN max_lsn);

        types::DataPage*
        create_dp_impl(const types::MetaTable& mt);

        types::DataPage*
        mark_dirty_impl(const types::DataPageId& page_id);
    };
} // namespace storage

#endif // DELTABASE_BUFFER_POOL_HPP
