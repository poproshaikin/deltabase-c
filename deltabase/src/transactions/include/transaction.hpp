//
// Created by poproshaikin on 07.03.26.
//

#ifndef DELTABASE_TRANSACTION_HPP
#define DELTABASE_TRANSACTION_HPP
#include "../../storage/include/buffer_pool.hpp"
#include "../../types/include/UUID.hpp"
#include "../../types/include/wal_log.hpp"
#include "../../wal/include/wal_manager.hpp"

#include <cstdint>

namespace txn
{
    using TxnId = types::UUID;

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
        storage::BufferPool& buffer_pool_;
        TransactionState state_ = TransactionState::IDLE;
        types::LSN last_lsn_ = 0;

        Transaction(const TxnId& id, wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool);

        friend class TransactionManager;

    public:
        TxnId
        get_id() const;

        types::LSN
        get_last_lsn() const;

        void
        begin();

        void
        append_log(const types::WALRecord& record);

        void
        commit();
    };
} // namespace txn

#endif // DELTABASE_TRANSACTION_HPP
