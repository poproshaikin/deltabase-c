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
#include "../../transactions/include/transaction.hpp"

#include <optional>

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
        prepare_dp(size_t size, const types::MetaTable& mt, txn::Transaction& txn);

        types::DataPage*
        find_row_owner(types::RowId row_id, const types::MetaTable& mt);

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

        void
        log_page_linking(
            types::DataPage* page,
            const types::DataPageId& next,
            const types::MetaTable& mt,
            txn::Transaction& txn);

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

        std::optional<types::LSN>
        insert_row_locked(
            const types::DataPageId& page_id,
            types::MetaTable& mt,
            const types::DataRow& row,
            txn::Transaction& txn);

        std::optional<types::LSN>
        delete_row_locked(
            const types::DataPageId& page_id,
            types::RowId row_id,
            types::MetaTable& mt,
            txn::Transaction& txn);

        void
        flush_dirty();
        void
        flush_dirty(types::LSN max_lsn);

        std::vector<std::pair<types::UUID, types::LSN>>
        snapshot_dpt();

    private:
        DataPageBuffer data_pages_;
        IndexFileBuffer index_files_;

        IIOManager& io_;

        std::mutex mutex_;

        std::unordered_map<types::TableId, std::vector<types::DataPageId>> data_pages_per_table_;
        std::unordered_map<types::TableId, std::vector<types::IndexId>> index_files_per_table_;

        std::unordered_map<types::UUID, types::LSN> dpt_;
        std::mutex dpt_mutex_;

        // Scratch slots: when a Cache::put() call made from inside an _impl
        // method evicts a dirty victim, the _impl stashes it here instead of
        // flushing it itself (flushing is I/O and must not happen while
        // mutex_ is held). The owning public method drains and writes these
        // out after it has released mutex_. At most one of each is ever
        // populated per public call, since each public method's _impl chain
        // calls Cache::put at most once.
        std::optional<types::DataPage> pending_dp_eviction_;
        std::optional<types::IndexFile> pending_if_eviction_;

        void
        flush_pending_evictions();

        // All methods suffixed with impl assume the caller holds the appropriate
        // locks on the data structures (data_pages/index_files_/data_pages_per_table_/index_files_per_table_).
        // *_impl methods may call other *_impl methods.
        // The locking public API (below) may not be called from any of these methods,
        // as doing so would cause the public methods to acquire the same lock, which would cause deadlock.

        void
        initialize_impl();

        void
        put_dp_impl(const types::DataPageId& page_id, types::DataPage&& page);

        types::DataPage*
        get_dp_impl(const types::DataPageId& page_id);

        types::DataPage*
        prepare_dp_impl(size_t size, const types::MetaTable& mt, txn::Transaction& txn);

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

        void
        log_page_linking_impl(
            types::DataPage* page,
            const types::DataPageId& next,
            const types::MetaTable& mt,
            txn::Transaction& txn);

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