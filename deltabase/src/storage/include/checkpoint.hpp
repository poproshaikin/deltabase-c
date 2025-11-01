#pragma once

#include "pages/page_buffers.hpp"
#include "wal/wal_manager.hpp"

namespace storage {
    class CheckpointManager {
        static constexpr std::chrono::duration<int64_t> interval = std::chrono::minutes(5);

        WalManager& wal_manager_;
        PageBuffers& buffers_;

        std::thread worker_thread_;

        void
        run_bg_worker();
    public:
        CheckpointManager(WalManager& wal, PageBuffers& buffers);
    };
}