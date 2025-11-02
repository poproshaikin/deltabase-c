#include "include/checkpoint.hpp"
#include "pages/page_buffers.hpp"
#include "wal/wal_manager.hpp"

#include <thread>

namespace storage {
    CheckpointManager::CheckpointManager(WalManager& wal, PageBuffers& buffers)
        : wal_manager_(wal), buffers_(buffers), stop_worker_(false)
    {
        run_bg_worker();
    }

    CheckpointManager::~CheckpointManager() 
    {
        stop_worker_ = true;
        if (worker_thread_.joinable())
            worker_thread_.join();
    }

    void
    CheckpointManager::run_bg_worker() {
        auto worker = [this]()
        {
            while (!stop_worker_)
            {
                std::this_thread::sleep_for(interval);
                wal_manager_.flush();
                buffers_.flush();
            }
        };

        worker_thread_ = std::thread(worker);
    }
}