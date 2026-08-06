//
// Created by poproshaikin on 8/3/26.
//

#ifndef DELTABASE_FLUSH_COORDINATOR_HPP
#define DELTABASE_FLUSH_COORDINATOR_HPP
#include "buffer_pool.hpp"
#include "catalog.hpp"
#include "wal_manager.hpp"

#include <thread>

namespace storage
{
    class FlushCoordinator
    {
    public:
        FlushCoordinator(
            BufferPool& buffer_pool,
            CatalogCache& catalog,
            wal::IWALManager& wal_manager,
            int interval_ms = 200);

        ~FlushCoordinator();

        void
        flush();
        void
        flush(types::LSN max_lsn);

    private:
        BufferPool& buffer_pool_;
        CatalogCache& catalog_;
        wal::IWALManager& wal_manager_;
        int interval_ms_;

        std::thread bg_thread_;
        std::atomic_bool stop_bg_thread_ = false;

        void
        run_bg_worker();
    };
}

#endif //DELTABASE_FLUSH_COORDINATOR_HPP