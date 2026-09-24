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
    Transaction::advance_lsn(types::LSN lsn)
    {
        last_lsn_ = lsn;
        mgr_->assign_active_entry(*this);
    }

    void
    Transaction::begin()
    {
        if (state_ != TransactionState::IDLE)
            throw std::runtime_error("Transaction::begin: transaction state not idle");

        types::BeginTxnRecord record(0, last_lsn_, id_);
        types::LSN lsn = mgr_->wal_manager().append_log(record);

        state_ = TransactionState::ACTIVE;
        advance_lsn(lsn);
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

        advance_lsn(mgr_->wal_manager().append_log(record_with_txn_id));
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

        mgr_->remove_active_entry(*this);
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

        mgr_->remove_active_entry(*this);
    }

    template <typename R, typename CLR>
    types::LSN
    Transaction::undo_page_record(const R& r, CLR clr)
    {
        if (auto* page = mgr_->buffer_pool().get_dp(r.page_id))
        {
            advance_lsn(mgr_->wal_manager().append_log(clr));
            mgr_->recovery_manager().undo_record(r, *page, last_lsn_);
            mgr_->buffer_pool().dirty_dp(r.page_id);
        }

        return r.prev_lsn;
    }

    template <typename R, typename CLR>
    types::LSN
    Transaction::undo_catalog_record(const R& r, CLR clr)
    {
        advance_lsn(mgr_->wal_manager().append_log(clr));
        mgr_->recovery_manager().undo_record(r, mgr_->catalog(), last_lsn_);
        return r.prev_lsn;
    }

    types::LSN
    Transaction::undo_one(const types::BeginTxnRecord&)
    {
        return 0;
    }

    types::LSN
    Transaction::undo_one(const types::InsertRecord& r)
    {
        return undo_page_record(
            r, types::CLRInsertRecord(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.after)
        );
    }

    types::LSN
    Transaction::undo_one(const types::UpdateRecord& r)
    {
        return undo_page_record(
            r,
            types::CLRUpdateRecord(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.before, r.after)
        );
    }

    types::LSN
    Transaction::undo_one(const types::DeleteRecord& r)
    {
        return undo_page_record(
            r, types::CLRDeleteRecord(0, last_lsn_, id_, r.table_id, r.page_id, r.prev_lsn, r.before)
        );
    }

    types::LSN
    Transaction::undo_one(const types::CreateSchemaRecord& r)
    {
        return undo_catalog_record(r, types::CLRCreateSchemaRecord(0, last_lsn_, id_, r.prev_lsn, r.schema));
    }

    types::LSN
    Transaction::undo_one(const types::UpdateSchemaRecord& r)
    {
        return undo_catalog_record(
            r, types::CLRUpdateSchemaRecord(0, last_lsn_, id_, r.prev_lsn, r.before, r.after)
        );
    }

    types::LSN
    Transaction::undo_one(const types::DeleteSchemaRecord& r)
    {
        return undo_catalog_record(r, types::CLRDeleteSchemaRecord(0, last_lsn_, id_, r.prev_lsn, r.before));
    }

    types::LSN
    Transaction::undo_one(const types::CreateTableRecord& r)
    {
        return undo_catalog_record(r, types::CLRCreateTableRecord(0, last_lsn_, id_, r.prev_lsn, r.after));
    }

    types::LSN
    Transaction::undo_one(const types::UpdateTableRecord& r)
    {
        return undo_catalog_record(
            r, types::CLRUpdateTableRecord(0, last_lsn_, id_, r.prev_lsn, r.before, r.after)
        );
    }

    types::LSN
    Transaction::undo_one(const types::DeleteTableRecord& r)
    {
        return undo_catalog_record(r, types::CLRDeleteTableRecord(0, last_lsn_, id_, r.prev_lsn, r.before));
    }

    types::LSN
    Transaction::undo_one(const types::CreateIndexRecord& r)
    {
        return undo_catalog_record(r, types::CLRCreateIndexRecord(0, last_lsn_, id_, r.prev_lsn, r.after));
    }

    types::LSN
    Transaction::undo_one(const types::DropIndexRecord& r)
    {
        return undo_catalog_record(r, types::CLRDropIndexRecord(0, last_lsn_, id_, r.prev_lsn, r.before));
    }

    types::LSN
    Transaction::undo_one(const types::CreateSequenceRecord& r)
    {
        return undo_catalog_record(r, types::CLRCreateSequenceRecord(0, last_lsn_, id_, r.prev_lsn, r.after));
    }

    types::LSN
    Transaction::undo_one(const types::UpdateSequenceRecord& r)
    {
        return undo_catalog_record(
            r, types::CLRUpdateSequenceRecord(0, last_lsn_, id_, r.prev_lsn, r.before, r.after)
        );
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
