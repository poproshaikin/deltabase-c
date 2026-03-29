//
// Created by poproshaikin on 08.03.26.
//

#include "include/transaction_manager.hpp"

#include "../storage/include/buffer_pool.hpp"
namespace txn
{
    TransactionManager::TransactionManager(
        wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool
    )
        : wal_manager_(wal_manager), buffer_pool_(buffer_pool)
    {
    }

    Transaction
    TransactionManager::make_transaction() const
    {
        return Transaction(TxnId::make(), wal_manager_, buffer_pool_);
    }
} // namespace txn
