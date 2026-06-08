//
// Created by poproshaikin on 07.03.26.
//

#include "include/transaction.hpp"

#include <stdexcept>
#include <variant>

namespace txn
{
    Transaction::Transaction(
        const TxnId& id, wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool,
        storage::CatalogCache& catalog, recovery::RecoveryManager& recovery_manager
    )
        : id_(id), wal_manager_(&wal_manager), buffer_pool_(&buffer_pool), catalog_(&catalog),
          recovery_manager_(&recovery_manager)
    {
    }

    TxnId
    Transaction::get_id() const
    {
        return id_;
    }

    types::LSN
    Transaction::get_last_lsn() const
    {
        return last_lsn_;
    }

    void
    Transaction::begin()
    {
        if (state_ != TransactionState::IDLE)
            throw std::runtime_error("Transaction::begin: transaction state not idle");

        types::BeginTxnRecord record(0, last_lsn_, id_);

        last_lsn_ = wal_manager_->append_log(record);
        state_ = TransactionState::ACTIVE;
    }

    void
    Transaction::append_log(const types::WALRecord& record)
    {
        if (state_ != TransactionState::ACTIVE)
            throw std::runtime_error("Transaction::append_log: transaction state not active");

        types::WALRecord record_with_txn_id = std::visit(
            [this](auto rec) -> types::WALRecord
            {
                rec.txn_id = id_;
                rec.prev_lsn = last_lsn_;
                return rec;
            },
            record
        );

        last_lsn_ = wal_manager_->append_log(record_with_txn_id);
    }

    void
    Transaction::commit()
    {
        if (state_ != TransactionState::ACTIVE)
            throw std::runtime_error("Transaction::commit: transaction state not active");

        types::CommitTxnRecord commit_record(0, last_lsn_, id_);

        last_lsn_ = wal_manager_->append_log(commit_record);
        wal_manager_->wait_for_durable(last_lsn_);
        buffer_pool_->flush_dirty(last_lsn_);
        catalog_->commit_txn(id_);
        state_ = TransactionState::COMMITTED;
    }

    void
    Transaction::rollback()
    {
        if (state_ != TransactionState::ACTIVE)
            throw std::runtime_error("Transaction::rollback: transaction state not active");

        types::LSN current = last_lsn_;

        while (current != 0)
        {
            auto record = wal_manager_->read_log(current);

            bool stop = false;

            std::visit([&](auto& r)
            {
                using R = std::decay_t<decltype(r)>;

                if constexpr (std::is_same_v<R, types::BeginTxnRecord>)
                {
                    stop = true;
                }
                else if constexpr (std::is_same_v<R, types::InsertRecord>)
                {
                    if (auto* page = buffer_pool_->get_dp(r.page_id))
                        recovery_manager_->undo_record(r, *page);

                    types::CLRInsertRecord clr(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.after);
                    last_lsn_ = wal_manager_->append_log(clr);
                    current = r.prev_lsn;
                }
                else if constexpr (std::is_same_v<R, types::UpdateRecord>)
                {
                    if (auto* page = buffer_pool_->get_dp(r.page_id))
                        recovery_manager_->undo_record(r, *page);

                    types::CLRUpdateRecord clr(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.before, r.after);
                    last_lsn_ = wal_manager_->append_log(clr);
                    current = r.prev_lsn;
                }
                else if constexpr (std::is_same_v<R, types::DeleteRecord>)
                {
                    if (auto* page = buffer_pool_->get_dp(r.page_id))
                        recovery_manager_->undo_record(r, *page);

                    types::CLRDeleteRecord clr(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.before);
                    last_lsn_ = wal_manager_->append_log(clr);
                    current = r.prev_lsn;
                }
                else if constexpr (requires { r.undo_next_lsn; })
                {
                    current = r.undo_next_lsn;
                }
                else
                {
                    current = r.prev_lsn;
                }
            }, record);

            if (stop) break;
        }

        types::RollbackTxnRecord rollback_record(0, last_lsn_, id_);
        last_lsn_ = wal_manager_->append_log(rollback_record);
        wal_manager_->wait_for_durable(last_lsn_);

        buffer_pool_->rollback_txn(id_);
        catalog_->rollback_txn(id_);
        state_ = TransactionState::ABORTED;
    }
} // namespace txn
