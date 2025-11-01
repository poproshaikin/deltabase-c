#include "include/checkpoint.hpp"
#include "pages/page_buffers.hpp"
#include "wal/wal_manager.hpp"
#include <thread>

namespace storage {
    CheckpointManager::CheckpointManager(WalManager& wal, PageBuffers& buffers)
        : wal_manager_(wal), buffers_(buffers) {
    }

    void
    CheckpointManager::run_bg_worker() {
        auto worker = [this]() {
            while (true) {
                std::this_thread::sleep_for(interval);
                wal_manager_.flush_on_disk();
                buffers_.flush();
            }
        };

        worker_thread_ = std::thread(worker);
    }
}