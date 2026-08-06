//
// Created by poproshaikin on 07.03.26.
//

#include "include/transaction.hpp"
#include "include/transaction_manager.hpp"

#include <stdexcept>
#include <variant>

namespace txn
{
    Transaction::Transaction(const TxnId& id, TransactionManager& mgr)
        : id_(id), mgr_(&mgr)
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

        last_lsn_ = mgr_->wal_manager().append_log(record);
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

        last_lsn_ = mgr_->wal_manager().append_log(record_with_txn_id);
    }

    void
    Transaction::commit()
    {
        if (state_ != TransactionState::ACTIVE)
            throw std::runtime_error("Transaction::commit: transaction state not active");

        types::CommitTxnRecord commit_record(0, last_lsn_, id_);

        last_lsn_ = mgr_->wal_manager().append_log(commit_record);
        mgr_->wal_manager().ensure_durable(last_lsn_);
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
            auto record = mgr_->wal_manager().read_log(current);
            current = std::visit([this](auto& r) -> types::LSN { return undo_one(r); }, record);
        }

        types::RollbackTxnRecord rollback_record(0, last_lsn_, id_);
        last_lsn_ = mgr_->wal_manager().append_log(rollback_record);
        mgr_->wal_manager().ensure_durable(last_lsn_);

        state_ = TransactionState::ABORTED;
    }

    types::LSN
    Transaction::undo_one(const types::BeginTxnRecord&)
    {
        return 0;
    }

    types::LSN
    Transaction::undo_one(const types::InsertRecord& r)
    {
        if (auto* page = mgr_->buffer_pool().get_dp(r.page_id))
        {
            types::CLRInsertRecord clr(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.after);
            last_lsn_ = mgr_->wal_manager().append_log(clr);

            mgr_->recovery_manager().undo_record(r, *page, last_lsn_);
            mgr_->buffer_pool().dirty_dp(r.page_id);
        }

        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::UpdateRecord& r)
    {
        if (auto* page = mgr_->buffer_pool().get_dp(r.page_id))
        {
            types::CLRUpdateRecord clr(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.before, r.after);
            last_lsn_ = mgr_->wal_manager().append_log(clr);

            mgr_->recovery_manager().undo_record(r, *page, last_lsn_);
            mgr_->buffer_pool().dirty_dp(r.page_id);
        }

        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::DeleteRecord& r)
    {
        if (auto* page = mgr_->buffer_pool().get_dp(r.page_id))
        {
            types::CLRDeleteRecord clr(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.before);
            last_lsn_ = mgr_->wal_manager().append_log(clr);

            mgr_->recovery_manager().undo_record(r, *page, last_lsn_);
            mgr_->buffer_pool().dirty_dp(r.page_id);
        }

        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::CreateSchemaRecord& r)
    {
        types::CLRCreateSchemaRecord clr(0, last_lsn_, id_, r.prev_lsn, r.schema);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::UpdateSchemaRecord& r)
    {
        types::CLRUpdateSchemaRecord clr(0, last_lsn_, id_, r.prev_lsn, r.before, r.after);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::DeleteSchemaRecord& r)
    {
        types::CLRDeleteSchemaRecord clr(0, last_lsn_, id_, r.prev_lsn, r.before);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::CreateTableRecord& r)
    {
        types::CLRCreateTableRecord clr(0, last_lsn_, id_, r.prev_lsn, r.after);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::UpdateTableRecord& r)
    {
        types::CLRUpdateTableRecord clr(0, last_lsn_, id_, r.prev_lsn, r.before, r.after);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::DeleteTableRecord& r)
    {
        types::CLRDeleteTableRecord clr(0, last_lsn_, id_, r.prev_lsn, r.before);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::CreateIndexRecord& r)
    {
        types::CLRCreateIndexRecord clr(0, last_lsn_, id_, r.prev_lsn, r.after);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::DropIndexRecord& r)
    {
        types::CLRDropIndexRecord clr(0, last_lsn_, id_, r.prev_lsn, r.before);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::CreateSequenceRecord& r)
    {
        types::CLRCreateSequenceRecord clr(0, last_lsn_, id_, r.prev_lsn, r.after);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::UpdateSequenceRecord& r)
    {
        types::CLRUpdateSequenceRecord clr(0, last_lsn_, id_, r.prev_lsn, r.before, r.after);
        last_lsn_ = mgr_->wal_manager().append_log(clr);
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    template <typename R>
    types::LSN
    Transaction::undo_one(const R& r)
    {
        if constexpr (requires { r.undo_next_lsn; })
            return r.undo_next_lsn;
        else
            return r.prev_lsn;
    }
} // namespace txn
