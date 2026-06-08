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

    public:
        TransactionManager(wal::IWALManager& wal_manager, storage::BufferPool& buffer_pool, storage::CatalogCache& catalog, recovery::RecoveryManager& recovery_manager);

        Transaction
        make_transaction() const;
    };
}

#endif // DELTABASE_TRANSACTION_MANAGER_HPP
