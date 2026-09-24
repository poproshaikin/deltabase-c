//
// Created by poproshaikin on 08.03.26.
//

#include "include/transaction_manager.hpp"
#include "../storage/include/buffer_pool.hpp"

namespace txn
{
    TransactionManager::TransactionManager(
        wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool,
        storage::CatalogCache& catalog, recovery::RecoveryManager& recovery_manager
    )
        : wal_manager_(wal_manager), buffer_pool_(buffer_pool), catalog_(catalog),
          recovery_manager_(recovery_manager)
    {
    }

    Transaction
    TransactionManager::make_transaction()
    {
        return Transaction(TxnId::make(), *this);
    }

    wal::IWALManager&
    TransactionManager::wal_manager() const
    {
        return wal_manager_;
    }

    storage::BufferPool&
    TransactionManager::buffer_pool() const
    {
        return buffer_pool_;
    }

    storage::CatalogCache&
    TransactionManager::catalog() const
    {
        return catalog_;
    }

    recovery::RecoveryManager&
    TransactionManager::recovery_manager() const
    {
        return recovery_manager_;
    }

    void
    TransactionManager::assign_active_entry(const Transaction& txn)
    {
        std::lock_guard lock(active_transactions_mutex_);
        active_transactions_.insert_or_assign(txn.id_, ActiveTxnEntry(txn.state_, txn.last_lsn_));
    }

    void
    TransactionManager::remove_active_entry(const Transaction& txn)
    {
        std::lock_guard lock(active_transactions_mutex_);
        active_transactions_.erase(txn.id_);
    }

    std::unordered_map<types::TxnId, TransactionManager::ActiveTxnEntry>
    TransactionManager::snapshot_att() const
    {
        std::unordered_map<types::TxnId, ActiveTxnEntry> active_transactions_copy;
        {
            std::lock_guard lock(active_transactions_mutex_);
            active_transactions_copy = active_transactions_;
        }
        return active_transactions_copy;
    }
} // namespace txn
