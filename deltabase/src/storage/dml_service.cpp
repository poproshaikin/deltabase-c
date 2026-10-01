//
// Created by poproshaikin on 6/11/26.
//

#include "dml_service.hpp"
#include "../types/include/meta_index.hpp"
#include "../misc/include/utils.hpp"
#include "BP_index_pager.hpp"
#include "index_bplus_tree.hpp"
#include "../misc/include/exceptions.hpp"

#include <unordered_set>

namespace storage
{
    using namespace types;
    using namespace misc;

    DMLService::DMLService(
        DDLService& ddl_service,
        BufferPool& buffer_pool,
        IIOManager& io_manager
    ) : ddl_service_(ddl_service),
        buffer_pool_(buffer_pool),
        io_manager_(io_manager)
    {
    }

    bool
    DMLService::is_row_obsolete(const RowPtr& row_ptr) const
    {
        return buffer_pool_.is_row_obsolete(row_ptr);
    }

    void
    DMLService::check_row_constraints(const MetaTable& mt, const DataRow& row)
    {
        for (auto& mi : mt.indexes)
        {
            const auto col_idx = mt.get_column_idx(mi.column_id);
            if (col_idx < 0)
                throw std::runtime_error("Index column not found in table schema");

            const auto& key = row.tokens[static_cast<size_t>(col_idx)];

            if (key.type == DataType::_NULL)
            {
                const auto& indexed_col = mt.get_column(mi.column_id);
                if (indexed_col.has_constraint<MetaPrimaryKeyConstraint>())
                    throw EngineException(
                        "PRIMARY KEY column cannot be NULL",
                        EngineException::Code::NOT_NULL_VIOLATION);
                continue;
            }

            if (mi.is_unique)
            {
                BPIndexPager pager(buffer_pool_, mt.id, mi.id);
                IndexBPlusTree tree(pager);

                auto existing = tree.find(key);
                if (existing.has_value() && !is_row_obsolete(existing.value()))
                    throw EngineException(
                        "Unique constraint violation: " + mi.name,
                        EngineException::Code::UNIQUE_VIOLATION);
            }
        }
    }

    std::vector<IndexId>
    DMLService::insert_row_into_indexes(
        const MetaTable& mt,
        const DataRow& row,
        const DataPageId& page_id,
        txn::Transaction& txn)
    {
        std::vector<IndexId> touched_indexes;
        touched_indexes.reserve(mt.indexes.size());

        for (auto& mi : mt.indexes)
        {
            const auto col_idx = mt.get_column_idx(mi.column_id);
            if (col_idx < 0)
                throw std::runtime_error("Index column not found in table schema");

            const auto& key = row.tokens[static_cast<size_t>(col_idx)];
            if (key.type == DataType::_NULL)
                continue;

            const RowPtr row_ptr{page_id, row.id};

            BPIndexPager pager(buffer_pool_, mt.id, mi.id);
            IndexBPlusTree tree(pager);

            tree.insert(key, row_ptr, txn);
            touched_indexes.push_back(mi.id);
        }

        return touched_indexes;
    }

    void
    DMLService::insert_row(
        MetaTable& mt,
        std::vector<DataToken> normalized_row,
        txn::Transaction& txn)
    {
        auto new_row = mt.make_row(normalized_row);
        check_row_constraints(mt, new_row);

        size_t row_size = io_manager_.estimate_size(new_row);

        while (true)
        {
            auto* page = buffer_pool_.prepare_dp(row_size, mt, txn);

            std::optional<LSN> result = buffer_pool_.insert_row_locked(
                page->id,
                mt,
                new_row,
                txn);

            if (!result.has_value())
                continue;

            std::vector<IndexId> touched_indexes;
            if (mt.indexes.size() > 0)
                touched_indexes = insert_row_into_indexes(mt, new_row, page->id, txn);

            for (const auto& index_id : touched_indexes)
                buffer_pool_.set_if_lsn(index_id, result.value());

            return;
        }
    }

    DataRow
    DMLService::apply_row_update(
        const MetaTable& mt,
        const DataRow& old_row,
        const RowUpdate& update)
    {
        DataRow new_row = old_row;
        for (const auto& assignment : update)
        {
            int64_t col_idx = mt.get_column_idx(
                std::visit([](auto& a)
                           {
                               return a.first;
                           },
                           assignment));

            if (const auto* lit = std::get_if<AssignLiteral>(&assignment))
                new_row.tokens[col_idx] = lit->second;
            else
            {
                const auto* col = std::get_if<AssignColumn>(&assignment);
                new_row.tokens[col_idx] = old_row.tokens[mt.get_column_idx(col->second)];
            }
        }
        return new_row;
    }

    void
    DMLService::update_one_row(
        MetaTable& mt,
        const DataPageId& page_id,
        RowId row_id,
        const RowUpdate& update,
        txn::Transaction& txn)
    {
        auto owner_page = buffer_pool_.get_dp(page_id);
        auto old_row_it = std::ranges::find(owner_page->rows, row_id, &DataRow::id);

        if (old_row_it == owner_page->rows.end())
            throw std::runtime_error("update_one_row: row wasn't found on given page");

        DataRow old_row = *old_row_it;
        DataRow new_row = apply_row_update(mt, old_row, update);
        new_row.id = ++mt.last_rid;

        check_row_constraints(mt, new_row);

        auto deleted = buffer_pool_.delete_row_locked(page_id, row_id, mt, txn);
        if (!deleted.has_value())
            return;

        DataPage *dest = nullptr;
        std::optional<LSN> inserted;
        while (!inserted.has_value())
        {
            dest = buffer_pool_.prepare_dp(io_manager_.estimate_size(new_row), mt, txn);
            inserted = buffer_pool_.insert_row_locked(dest->id, mt, new_row, txn);
        }

        if (!mt.indexes.empty())
        {
            auto touched = insert_row_into_indexes(mt, new_row, dest->id, txn);
            for (const auto& index_id : touched)
                buffer_pool_.set_if_lsn(index_id, inserted.value());
        }
    }

    void
    DMLService::update_selected(
        MetaTable& mt,
        RowUpdate update,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn)
    {
        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        // Phase 1: read-only, no locked-mutations
        std::vector<std::pair<DataPageId, RowId>> candidates;
        for (DataPage* page : buffer_pool_.get_table_data(mt.id))
            for (const auto& row : page->rows)
                if (ids.contains(row.id) && !has_flag(row.flags, DataRowFlags::OBSOLETE))
                    candidates.emplace_back(page->id, row.id);

        // Phase 2: mutation. DO NOT USE DataPage* - may become mutated by someone else.
        for (const auto& [page_id, row_id] : candidates)
            update_one_row(mt, page_id, row_id, update, txn);
    }

    void
    DMLService::delete_selected(
        MetaTable& mt,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn)
    {
        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        // Phase 1: read-only
        std::vector<std::pair<DataPageId, RowId>> candidates;
        for (DataPage* page : buffer_pool_.get_table_data(mt.id))
            for (const auto& row : page->rows)
                if (ids.contains(row.id) && !has_flag(row.flags, DataRowFlags::OBSOLETE))
                    candidates.emplace_back(page->id, row.id);

        // Phase 2
        for (const auto& [page_id, row_id] : candidates)
            buffer_pool_.delete_row_locked(page_id, row_id, mt, txn);
    }


}