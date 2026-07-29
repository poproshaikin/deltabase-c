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
        const auto* page = buffer_pool_.get_dp(row_ptr.first);
        if (!page)
            return false;

        for (auto it = page->rows.rbegin(); it != page->rows.rend(); ++it)
        {
            const auto& row = *it;
            if (row.id == row_ptr.second)
                return has_flag(row.flags, DataRowFlags::OBSOLETE);
        }

        return false;
    }

    std::vector<IndexId>
    DMLService::insert_row_into_indexes(
        const MetaTable& mt,
        const DataRow& row,
        const DataPageId& page_id)
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
            {
                const auto& indexed_col = mt.get_column(mi.column_id);
                if (indexed_col.has_constraint<MetaPrimaryKeyConstraint>())
                    throw EngineException(
                        "PRIMARY KEY column cannot be NULL",
                        EngineException::Code::NOT_NULL_VIOLATION);
                continue;
            }

            const RowPtr row_ptr{page_id, row.id};

            BPIndexPager pager(buffer_pool_, mt.id, mi.id);
            IndexBPlusTree tree(pager);

            if (mi.is_unique)
            {
                auto existing = tree.find(key);
                if (existing.has_value() && !is_row_obsolete(existing.value()))
                    throw EngineException("Unique constraint violation: " + mi.name,
                                          EngineException::Code::UNIQUE_VIOLATION);
            }

            tree.insert(key, row_ptr);
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
        size_t row_size = io_manager_.estimate_size(new_row);

        const auto mt_unchanged = mt;

        auto* page = buffer_pool_.prepare_dp(row_size, mt, txn.get_id());

        std::vector<IndexId> touched_indexes;
        if (mt.indexes.size() > 0)
            touched_indexes = insert_row_into_indexes(mt, new_row, page->id);

        InsertRecord insert_record(mt.id, page->id, new_row);
        UpdateTableRecord update_table_record(mt_unchanged, mt);
        txn.append_log(insert_record);
        txn.append_log(update_table_record);
        const LSN page_lsn = txn.get_last_lsn();

        buffer_pool_.append_row(page, mt, new_row, page_lsn, txn.get_id());

        for (const auto& index_id : touched_indexes)
            buffer_pool_.set_if_lsn(index_id, page_lsn);
    }

    void
    DMLService::update_selected(
        types::MetaTable& mt,
        RowUpdate update,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn)
    {
        const auto unchanged_mt = mt;
        auto pages = buffer_pool_.get_table_data(mt.id);

        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        for (DataPage* page : pages)
        {
            bool updated = false;
            LSN page_lsn = page->last_lsn;

            for (auto& row : page->rows)
            {
                if (!ids.contains(row.id))
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                DataRow new_row = row;
                new_row.id = ++mt.last_rid;

                row.flags |= DataRowFlags::OBSOLETE;

                for (const auto& assignment : update)
                {
                    ColumnId col_id = std::visit(
                        [](auto& a) { return a.first; }, assignment);

                    int64_t col_idx = mt.get_column_idx(col_id);
                    MetaColumn cola = mt.get_column(col_idx);

                    if (auto* lit = std::get_if<AssignLiteral>(&assignment))
                    {
                        new_row.tokens[col_idx] = lit->second;
                    }
                    else
                    {
                        auto* col = std::get_if<AssignColumn>(&assignment);
                        int src_idx = mt.get_column_idx(col->second);
                        new_row.tokens[col_idx] = row.tokens[src_idx];
                    }
                }

                mt.total_rows++;

                UpdateRecord update_record(mt.id, page->id, row, new_row);
                txn.append_log(update_record);
                page_lsn = std::max(page_lsn, txn.get_last_lsn());

                UpdateTableRecord update_table_record(unchanged_mt, mt);
                txn.append_log(update_table_record);

                page->rows.push_back(new_row);
                page->max_rid = std::max(page->max_rid, new_row.id);

                if (mt.indexes.size() > 0)
                {
                    auto touched_indexes = insert_row_into_indexes(mt, new_row, page->id);
                    for (const auto& index_id : touched_indexes)
                        buffer_pool_.set_if_lsn(index_id, page_lsn);
                }

                updated = true;
            }

            if (updated)
            {
                page->last_lsn = page_lsn;
                buffer_pool_.dirty_dp(page->id, txn.get_id());
            }
        }
    }

    void
    DMLService::delete_selected(
        MetaTable& mt,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn)
    {
        const auto unchanged_mt = mt;
        auto pages = buffer_pool_.get_table_data(mt.id);

        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        for (auto& page : pages)
        {
            bool deleted = false;
            LSN page_lsn = page->last_lsn;

            for (auto& row : page->rows)
            {
                if (!ids.contains(row.id))
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                deleted = true;

                row.flags |= DataRowFlags::OBSOLETE;

                mt.live_rows--;

                DeleteRecord record(mt.id, page->id, row);
                txn.append_log(record);
                page_lsn = std::max(page_lsn, txn.get_last_lsn());
                UpdateTableRecord update_table_record(unchanged_mt, mt);
                txn.append_log(update_table_record);
            }

            if (deleted)
            {
                page->last_lsn = page_lsn;
                buffer_pool_.dirty_dp(page->id, txn.get_id());
            }
        }
    }


}