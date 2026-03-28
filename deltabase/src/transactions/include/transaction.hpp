//
// Created by poproshaikin on 07.03.26.
//

#ifndef DELTABASE_TRANSACTION_HPP
#define DELTABASE_TRANSACTION_HPP
#include "../../types/include/uuid.hpp"
#include "../../types/include/wal_log.hpp"
#include "../../wal/include/wal_manager.hpp"

#include <stdexcept>
#include <variant>
#include <vector>

namespace txn
{
    using TxnId = types::Uuid;

    enum class TransactionState
    {
        IDLE = 0,
        ACTIVE,
        COMMITTED,
        ABORTED
    };

    class Transaction
    {
        TxnId id_;
        wal::IWALManager& wal_manager_;
        TransactionState state_;

        Transaction(const TxnId& id, wal::IWALManager& wal_manager)
            : id_(id), wal_manager_(wal_manager), state_(TransactionState::IDLE)
        {
        }

        friend class TransactionManager;

    public:
        TxnId
        get_id() const
        {
            return id_;
        }

        void
        begin()
        {
            if (state_ != TransactionState::IDLE)
                throw std::runtime_error("Transaction::begin: transaction state not idle");

            // lsn/prev_lsn will be assigned in the WAL manager
            types::BeginTxnRecord record(0, 0, id_);

            wal_manager_.append_log(record);
            wal_manager_.flush();
            state_ = TransactionState::ACTIVE;
        }

        void
        append_log(const types::WALRecord& record)
        {
            if (state_ != TransactionState::ACTIVE)
                throw std::runtime_error("Transaction::append_log: transaction state not active");

            types::WALRecord record_with_txn_id = std::visit([this](auto rec) -> types::WALRecord
            {
                rec.txn_id = id_;
                return rec;
            }, record);

            wal_manager_.append_log(record_with_txn_id);
            wal_manager_.flush();
        }

        void
        commit()
        {
            if (state_ != TransactionState::ACTIVE)
                throw std::runtime_error("Transaction::commit: transaction state not active");

            types::CommitTxnRecord commit_record(0, 0, id_); // lsn/prev_lsn assigned in WAL manager

            wal_manager_.append_log(commit_record);
            wal_manager_.flush();
            state_ = TransactionState::COMMITTED;
        }
    };
} // namespace txn

#endif // DELTABASE_TRANSACTION_HPP
