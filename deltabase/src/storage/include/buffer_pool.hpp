//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_BUFFER_POOL_HPP
#define DELTABASE_BUFFER_POOL_HPP

#include "../../misc/include/LRU_policy.hpp"
#include "../../misc/include/cache.hpp"
#include "../../types/include/data_page.hpp"
#include "../../types/include/index_page.hpp"
#include "io_manager.hpp"

namespace storage
{
    template <typename TKey, typename TValue>
    using Buffer = misc::Cache<TKey, TValue, cache::LRUPolicy<TKey>>;
    using DataPageBuffer = Buffer<types::DataPageId, types::DataPage>;
    using IndexPageBuffer = Buffer<types::IndexPageId, types::IndexPage>;

    class BufferPool
    {
        DataPageBuffer data_pages_;
        IndexPageBuffer index_pages_;
        IIOManager& io_;

        std::unordered_map<types::TableId, std::vector<types::DataPageId>>
            pages_to_tables_;

        void
        flush(DataPageBuffer::CacheEntry& page_entry);

        std::function<void(DataPageBuffer::CacheEntry&)> flusher_ =
            [this](DataPageBuffer::CacheEntry& page_entry) { flush(page_entry); };

        types::DataPage*
        create_dp(const types::MetaTable& mt);

    public:
        BufferPool(IIOManager& io) : data_pages_(cache::LRUPolicy<types::DataPageId>{}), index_pages_(cache::LRUPolicy<types::IndexPageId>{}), io_(io)
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
        get_table_data(const types::Uuid& table_id);

        std::vector<types::IndexPage*>
        get_table_index(const types::Uuid& table_id, const types::IndexId& index_id);

        types::DataPage*
        dirty_dp(const types::DataPageId& page_id);

        void
        flush_dirty();
    };
} // namespace cache

#endif // DELTABASE_BUFFER_POOL_HPP
