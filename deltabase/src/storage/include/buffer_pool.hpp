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

namespace storage
{
    template <typename TKey, typename TValue>
    using Buffer = misc::Cache<TKey, TValue, cache::LRUPolicy<TKey>>;
    using DataPageBuffer = Buffer<types::DataPageId, types::DataPage>;
    using IndexFileBuffer = Buffer<types::IndexId, types::IndexFile>;

    class BufferPool
    {
        DataPageBuffer data_pages_;
        IndexFileBuffer index_files_;
        IIOManager& io_;

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

        types::DataPage*
        create_dp(const types::MetaTable& mt);

    public:
        BufferPool(IIOManager& io)
            : data_pages_(cache::LRUPolicy<types::DataPageId>{}),
              index_files_(cache::LRUPolicy<types::IndexId>{}),
              io_(io)
        {
        }

        void
        initialize();

        void
        put_dp(const types::DataPageId& page_id, types::DataPage&& page);

        types::DataPage*
        get_dp(const types::DataPageId& page_id);

        types::DataPage*
        prepare_dp(size_t size, const types::MetaTable& mt);

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

        void
        flush_dirty();

        void
        flush_dirty(types::LSN max_lsn);
    };
} // namespace storage

#endif // DELTABASE_BUFFER_POOL_HPP
