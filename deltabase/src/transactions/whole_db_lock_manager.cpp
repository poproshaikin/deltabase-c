//
// Created by poproshaikin on 10/4/26.
//

#include "include/whole_db_lock_manager.hpp"

namespace txn
{
    void
    WholeDbLockManager::acquire(const TxnId& txn_id, const LockKey& key, LockMode mode)
    {
        if (mode == LockMode::Exclusive)
            mutex_.lock();
        else
            mutex_.lock_shared();

        std::lock_guard lock(state_mutex_);
        held_[txn_id] = mode;
    }

    void
    WholeDbLockManager::release_all(const TxnId& txn_id)
    {
        LockMode mode;
        {
            std::lock_guard lock(state_mutex_);
            auto it = held_.find(txn_id);
            if (it == held_.end())
                return;

            mode = it->second;
            held_.erase(it);
        }

        if (mode == LockMode::Exclusive)
            mutex_.unlock();
        else
            mutex_.unlock_shared();
    }
}
