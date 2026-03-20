//
// Created by poproshaikin on 08.03.26.
//

#include "include/transaction_manager.hpp"
namespace txn
{
    TransactionManager::TransactionManager(wal::IWalManager& wal_manager) : wal_manager_(wal_manager)
    {
    }

    Transaction
    TransactionManager::make_transaction() const
    {
        return Transaction(TxnId::make(), wal_manager_);
    }
} // namespace txn
