//
// Created by poproshaikin on 3/23/26.
//

#include "recovery_manager.hpp"

namespace recovery
{
    using namespace types;
    using namespace txn;

    RecoveryManager::RecoveryManager(Config& cfg, wal::IWALManager& wal, storage::IIOManager& io)
        : cfg_(cfg), wal_(wal), io_(io)
    {
    }

    void
    RecoveryManager::recover()
    {
        auto wal = wal_.read_all_logs();
        auto commit_lsns = get_commit_lsns(wal);
        auto rollback_lsns = get_rollback_lsns(wal);
        auto last_lsn_per_txn = get_last_lsns(wal);

        for (const auto& record : wal)
            redo(record, commit_lsns);

        auto active_txns = get_active_txns(last_lsn_per_txn, commit_lsns, rollback_lsns);
        undo(active_txns);
    }

    // ### REDO ###

    void
    RecoveryManager::redo(
        const WALRecord& record, const std::unordered_map<TxnId, LSN>& commit_lsns
    )
    {
        LSN last_checkpoint = cfg_.last_checkpoint_lsn;

        std::visit(
            [&]<typename TRecord>(const TRecord& r)
            {
                using R = std::decay_t<TRecord>;

                if constexpr (
                    std::is_same_v<R, CLRInsertRecord> ||
                    std::is_same_v<R, CLRUpdateRecord> ||
                    std::is_same_v<R, CLRDeleteRecord>)
                {
                    if (r.lsn <= last_checkpoint)
                        return;

                    redo_data(r);
                    return;
                }

                // REDO only committed txns
                if (!commit_lsns.contains(r.txn_id))
                    return;

                // Skip records not needing redo by checkpoint boundary
                if (r.lsn <= last_checkpoint)
                    return;

                // Safety: ignore records after commit marker (if malformed WAL)
                if (r.lsn > commit_lsns.at(r.txn_id))
                    return;

                if constexpr (wal_log::has_page_id_v<R>)
                    redo_data(r);
                else
                    redo_meta(r);
            },
            record
        );
    }
    void
    RecoveryManager::redo_data(const WALDataRecord& record)
    {
        std::visit(
            [&](const auto& r)
            {
                auto page = io_.read_data_page(r.page_id);
                auto mt = io_.read_table_meta(r.table_id);
                if (!page)
                    page = std::make_unique<DataPage>(io_.create_page(mt, r.page_id));

                if (page->header.last_lsn < r.lsn)
                {
                    redo(r, *page);
                    page->header.last_lsn = r.lsn;
                    io_.write_page(*page);
                }
            },
            record
        );
    }
    void
    RecoveryManager::redo_meta(const WALRecord& record)
    {
        std::visit(
            [&]<typename TRecord>(const TRecord& r)
            {
                using R = std::decay_t<TRecord>;
                if constexpr (is_in_variant_v<R, WALMetaRecord>)
                {
                    if constexpr (std::is_same_v<R, CreateSchemaRecord>)
                        io_.write_ms(r.schema);
                    else if constexpr (std::is_same_v<R, UpdateSchemaRecord>)
                        io_.write_ms(r.after);
                    else if constexpr (std::is_same_v<R, DeleteSchemaRecord>)
                        io_.delete_ms(r.before);
                    else if constexpr (std::is_same_v<R, CreateTableRecord>)
                        io_.write_mt(r.after);
                    else if constexpr (std::is_same_v<R, UpdateTableRecord>)
                        io_.write_mt(r.after);
                    else if constexpr (std::is_same_v<R, DeleteTableRecord>)
                        io_.delete_mt(r.before);
                }
            },
            record
        );
    }

    void
    RecoveryManager::redo(const InsertRecord& record, DataPage& page)
    {
        page.rows.push_back(record.after);
    }
    void
    RecoveryManager::redo(const UpdateRecord& record, DataPage& page)
    {
        bool found = false;

        for (auto& row : page.rows)
        {
            if (row.id != record.before.id)
                continue;

            row.flags |= DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(DeleteRecord): failed to find old row " +
                std::to_string(record.before.id)
            );

        page.rows.push_back(record.after);
    }
    void
    RecoveryManager::redo(const DeleteRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.before.id)
                continue;

            row.flags |= DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(DeleteRecord): failed to find old row " +
                std::to_string(record.before.id)
            );
    }

    void
    RecoveryManager::redo(const CLRInsertRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.after.id)
                continue;

            row.flags |= DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(CLRInsertRecord): failed to find row " +
                std::to_string(record.after.id)
            );
    }

    void
    RecoveryManager::redo(const CLRUpdateRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.after.id)
                continue;

            row.tokens = record.before.tokens;
            row.flags &= ~DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(CLRUpdateRecord): failed to find row " +
                std::to_string(record.after.id)
            );
    }

    void
    RecoveryManager::redo(const CLRDeleteRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.before.id)
                continue;

            row.flags &= ~DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(CLRDeleteRecord): failed to find row " +
                std::to_string(record.before.id)
            );
    }

    void
    RecoveryManager::undo(const std::unordered_map<TxnId, LSN>& active_lsns)
    {
        for (const auto& [txn_id, last_lsn] : active_lsns)
        {
            LSN current = last_lsn;
            LSN txn_prev_lsn = last_lsn;

            while (current != 0)
            {
                auto record = wal_.read_log(current);

                std::visit(
                    [&]<typename TRecord>(const TRecord& r)
                    {
                        using R = std::decay_t<TRecord>;

                        if constexpr (
                            std::is_same_v<R, CLRInsertRecord> ||
                            std::is_same_v<R, CLRUpdateRecord> ||
                            std::is_same_v<R, CLRDeleteRecord>)
                        {
                            txn_prev_lsn = r.lsn;
                            current = r.undo_next_lsn;
                            return;
                        }

                        else if constexpr (wal_log::has_page_id_v<R>)
                        {
                            auto page = io_.read_data_page(r.page_id);
                            if (!page)
                            {
                                current = r.prev_lsn;
                                return;
                            }

                            auto clr = make_clr(r);
                            clr = std::visit(
                                [&](auto rec) -> WALRecord
                                {
                                    rec.lsn = txn_prev_lsn;
                                    return rec;
                                },
                                clr
                            );

                            LSN clr_lsn = wal_.get_next_lsn();
                            wal_.append_log(clr);
                            wal_.flush();

                            undo_record(r, *page);
                            page->header.last_lsn = clr_lsn;
                            io_.write_page(*page);

                            txn_prev_lsn = clr_lsn;
                        }
                        else if constexpr (is_in_variant_v<R, WALMetaRecord>)
                        {
                            undo_record(r);
                        }
                        current = r.prev_lsn;
                    },
                    record
                );
            }

            RollbackTxnRecord rollback_marker(txn_prev_lsn, 0, txn_id);
            wal_.append_log(rollback_marker);
            wal_.flush();
        }
    }
    void
    RecoveryManager::undo_record(const InsertRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.after.id)
                continue;

            row.flags |= DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::undo(InsertRecord): failed to find 'after' row " +
                std::to_string(record.after.id)
            );
    }
    void
    RecoveryManager::undo_record(const UpdateRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.after.id)
                continue;

            row.tokens = record.before.tokens;
            row.flags &= ~DataRowFlags::OBSOLETE;
            found = true;
            break;
        }
        if (!found)
            throw std::runtime_error(
                "RecoveryManager::undo(UpdateRecord): failed to find 'after' row " +
                std::to_string(record.after.id)
            );
    }
    void
    RecoveryManager::undo_record(const DeleteRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.before.id)
                continue;

            row.flags &= ~DataRowFlags::OBSOLETE;
            found = true;
            break;
        }
        if (!found)
            throw std::runtime_error(
                "RecoveryManager::undo(DeleteRecord): failed to find 'after' row " +
                std::to_string(record.before.id)
            );
    }

    void
    RecoveryManager::undo_record(const CreateSchemaRecord& record)
    {
        io_.delete_ms(record.schema);
    }

    void
    RecoveryManager::undo_record(const UpdateSchemaRecord& record)
    {
        io_.write_ms(record.before);
    }

    void
    RecoveryManager::undo_record(const DeleteSchemaRecord& record)
    {
        io_.write_ms(record.before);
    }

    void
    RecoveryManager::undo_record(const CreateTableRecord& record)
    {
        io_.delete_mt(record.after);
    }

    void
    RecoveryManager::undo_record(const UpdateTableRecord& record)
    {
        io_.write_mt(record.before);
    }

    void
    RecoveryManager::undo_record(const DeleteTableRecord& record)
    {
        io_.write_mt(record.before);
    }

    WALRecord
    RecoveryManager::make_clr(const InsertRecord& record) const
    {
        return CLRInsertRecord(
            record.lsn,
            0,
            record.txn_id,
            record.table_id,
            record.page_id,
            record.prev_lsn,
            record.after
        );
    }

    WALRecord
    RecoveryManager::make_clr(const UpdateRecord& record) const
    {
        return CLRUpdateRecord(
            record.lsn,
            0,
            record.txn_id,
            record.table_id,
            record.page_id,
            record.prev_lsn,
            record.before,
            record.after
        );
    }

    WALRecord
    RecoveryManager::make_clr(const DeleteRecord& record) const
    {
        return CLRDeleteRecord(
            record.lsn,
            0,
            record.txn_id,
            record.table_id,
            record.page_id,
            record.prev_lsn,
            record.before
        );
    }

    std::unordered_map<TxnId, LSN>
    RecoveryManager::get_commit_lsns(const std::vector<WALRecord>& wal) const
    {
        std::unordered_map<TxnId, LSN> commit_lsns;

        for (const auto& record : wal)
        {
            std::visit(
                [&](const auto& rec)
                {
                    using R = std::decay_t<decltype(rec)>;

                    if constexpr (std::is_same_v<R, CommitTxnRecord>)
                    {
                        commit_lsns[rec.txn_id] = rec.lsn;
                    }
                },
                record
            );
        }
        return commit_lsns;
    }

    std::unordered_map<TxnId, LSN>
    RecoveryManager::get_rollback_lsns(const std::vector<WALRecord>& wal) const
    {
        std::unordered_map<TxnId, LSN> rollback_lsns;

        for (const auto& record : wal)
        {
            std::visit(
                [&](const auto& rec)
                {
                    using R = std::decay_t<decltype(rec)>;

                    if constexpr (std::is_same_v<R, RollbackTxnRecord>)
                    {
                        rollback_lsns[rec.txn_id] = rec.lsn;
                    }
                },
                record
            );
        }

        return rollback_lsns;
    }
    std::unordered_map<TxnId, LSN>
    RecoveryManager::get_last_lsns(const std::vector<WALRecord>& wal) const
    {
        std::unordered_map<TxnId, LSN> last;

        for (const auto& record : wal)
        {
            std::visit([&](const auto& r) { last[r.txn_id] = r.lsn; }, record);
        }

        return last;
    }
    std::unordered_map<TxnId, LSN>
    RecoveryManager::get_active_txns(
        const std::unordered_map<TxnId, LSN>& last,
        const std::unordered_map<TxnId, LSN>& commit,
        const std::unordered_map<TxnId, LSN>& rollback
    )
    {
        std::unordered_map<TxnId, LSN> active;

        for (const auto& [txn_id, lsn] : last)
        {
            if (!commit.contains(txn_id) && !rollback.contains(txn_id))
                active[txn_id] = lsn;
        }

        return active;
    }
} // namespace recovery