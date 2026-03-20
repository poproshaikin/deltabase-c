//
// Created by poproshaikin on 06.03.26.
//

#ifndef DELTABASE_TRANSACTION_MANAGER_HPP
#define DELTABASE_TRANSACTION_MANAGER_HPP
#include "transaction.hpp"

namespace txn
{
    class TransactionManager
    {
        wal::IWalManager& wal_manager_;

    public:
        TransactionManager(wal::IWalManager& wal_manager);

        Transaction
        make_transaction() const;
    };
}

#endif // DELTABASE_TRANSACTION_MANAGER_HPP
