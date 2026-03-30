//
// Created by poproshaikin on 07.03.26.
//

#include "include/transaction.hpp"

#include <stdexcept>
#include <thread>
#include <variant>

namespace txn
{
    Transaction::Transaction(
        const TxnId& id, wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool
    )
        : id_(id), wal_manager_(wal_manager), buffer_pool_(buffer_pool)
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

        // lsn/prev_lsn are assigned in the WAL manager.
        types::BeginTxnRecord record(0, last_lsn_, id_);

        wal_manager_.append_log(record);
        last_lsn_ = wal_manager_.get_next_lsn() - 1;
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

        wal_manager_.append_log(record_with_txn_id);
        last_lsn_ = wal_manager_.get_next_lsn() - 1;
    }

    void
    Transaction::commit()
    {
        if (state_ != TransactionState::ACTIVE)
            throw std::runtime_error("Transaction::commit: transaction state not active");

        types::CommitTxnRecord commit_record(0, last_lsn_, id_);

        wal_manager_.append_log(commit_record);
        last_lsn_ = wal_manager_.get_next_lsn() - 1;
        wal_manager_.flush();
        buffer_pool_.flush_dirty();
        state_ = TransactionState::COMMITTED;

        auto* buffer_pool = &buffer_pool_;
        // std::thread([buffer_pool] {
        //     buffer_pool->flush_dirty();
        // }).detach();
    }
} // namespace txn
