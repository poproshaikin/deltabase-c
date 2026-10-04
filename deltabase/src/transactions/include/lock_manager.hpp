//
// Created by poproshaikin on 10/3/26.
//

#ifndef DELTABASE_LOCK_MANAGER_HPP
#define DELTABASE_LOCK_MANAGER_HPP
#include "transaction.hpp"

namespace txn
{
    enum class LockMode { Shared, Exclusive };
    struct LockKey
    {
        // to be extended
    };

    class ILockManager
    {
    public:
        virtual ~ILockManager() = default;

        virtual void
        acquire(const TxnId& txn_id, const LockKey& key, LockMode mode) = 0;

        virtual void
        release_all(const TxnId& txn_id) = 0;
    };
}

#endif //DELTABASE_LOCK_MANAGER_HPP
