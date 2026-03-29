//
// Created by poproshaikin on 06.03.26.
//

#ifndef DELTABASE_TRANSACTION_MANAGER_HPP
#define DELTABASE_TRANSACTION_MANAGER_HPP
#include "../../storage/include/buffer_pool.hpp"
#include "transaction.hpp"

namespace txn
{
    class TransactionManager
    {
        wal::IWALManager& wal_manager_;
        storage::BufferPool& buffer_pool_;

    public:
        TransactionManager(wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool);

        Transaction
        make_transaction() const;
    };
}

#endif // DELTABASE_TRANSACTION_MANAGER_HPP
