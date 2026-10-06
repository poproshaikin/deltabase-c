//
// Created by poproshaikin on 10/4/26.
//

#ifndef DELTABASE_WHOLE_DB_LOCK_MANAGER_HPP
#define DELTABASE_WHOLE_DB_LOCK_MANAGER_HPP
#include "lock_manager.hpp"

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace txn
{
    class WholeDbLockManager : public ILockManager
    {
    public:
        void
        acquire(const TxnId& txn_id, const LockKey& key, LockMode mode) override;

        void
        release_all(const TxnId& txn_id) override;

    private:
        std::shared_mutex mutex_;

        std::mutex state_mutex_;
        std::unordered_map<TxnId, LockMode> held_;
    };
}

#endif //DELTABASE_WHOLE_DB_LOCK_MANAGER_HPP
