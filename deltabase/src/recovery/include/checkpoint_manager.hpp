//
// Created by poproshaikin on 9/24/26.
//

#ifndef DELTABASE_CHECKPOINT_MANAGER_HPP
#define DELTABASE_CHECKPOINT_MANAGER_HPP
#include "buffer_pool.hpp"
#include "wal_manager.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <thread>

namespace recovery
{
    class CheckpointManager
    {
    public:
        explicit CheckpointManager(
            wal::IWALManager& wal,
            storage::IIOManager& io,
            storage::BufferPool& buffer_pool,
            txn::TransactionManager& txn_manager);

        ~CheckpointManager();

        void
        start_background(std::chrono::milliseconds interval);

        void
        stop_background();

    private:
        std::atomic_bool running_;
        std::thread bg_thread_;
        std::mutex checkpoint_mtx_;

        std::mutex cv_mtx_;
        std::condition_variable cv_;

        wal::IWALManager& wal_;
        storage::IIOManager& io_;
        storage::BufferPool& buffer_pool_;
        txn::TransactionManager& txn_manager_;

        void
        do_checkpoint();
    };
}

#endif //DELTABASE_CHECKPOINT_MANAGER_HPP
