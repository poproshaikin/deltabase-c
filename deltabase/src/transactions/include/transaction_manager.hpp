//
// Created by poproshaikin on 06.03.26.
//

#ifndef DELTABASE_TRANSACTION_MANAGER_HPP
#define DELTABASE_TRANSACTION_MANAGER_HPP
#include "../../recovery/include/recovery_manager.hpp"
#include "../../storage/include/buffer_pool.hpp"
#include "../../storage/include/catalog.hpp"
#include "transaction.hpp"

namespace txn
{
    class TransactionManager
    {
        wal::IWALManager& wal_manager_;
        storage::BufferPool& buffer_pool_;
        storage::CatalogCache& catalog_;
        recovery::RecoveryManager& recovery_manager_;

        struct ActiveTxnEntry
        {
            TransactionState state;
            types::LSN last_lsn;

            explicit ActiveTxnEntry(TransactionState state, types::LSN last_lsn)
                : state(state), last_lsn(last_lsn)
            {
            }
        };

        std::unordered_map<types::TxnId, ActiveTxnEntry> active_transactions_;
        mutable std::mutex active_transactions_mutex_;

    public:
        TransactionManager(
            wal::IWALManager& wal_manager,
            storage::BufferPool& buffer_pool,
            storage::CatalogCache& catalog,
            recovery::RecoveryManager& recovery_manager);

        Transaction
        make_transaction();

        std::vector<std::pair<types::TxnId, types::LSN>>
        snapshot_att() const;

    private:
        wal::IWALManager&
        wal_manager() const;

        storage::BufferPool&
        buffer_pool() const;

        storage::CatalogCache&
        catalog() const;

        recovery::RecoveryManager&
        recovery_manager() const;

        void
        assign_active_entry(const Transaction& txn);

        void
        remove_active_entry(const Transaction& txn);

        friend class Transaction;
    };
}

#endif // DELTABASE_TRANSACTION_MANAGER_HPP