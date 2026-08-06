//
// Created by poproshaikin on 8/3/26.
//

#include "flush_coordinator.hpp"

namespace storage
{
    using namespace types;

    FlushCoordinator::FlushCoordinator(
        BufferPool& buffer_pool,
        CatalogCache& catalog,
        wal::IWALManager& wal_manager,
        int interval_ms)
        : buffer_pool_(buffer_pool), catalog_(catalog), wal_manager_(wal_manager),
          interval_ms_(interval_ms)
    {
        bg_thread_ = std::thread([this]
        {
            run_bg_worker();
        });
    }

    FlushCoordinator::~FlushCoordinator()
    {
        stop_bg_thread_ = true;
        flush();

        if (bg_thread_.joinable())
            bg_thread_.join();
    }

    void
    FlushCoordinator::run_bg_worker()
    {
        LSN last_durable_lsn = wal_manager_.get_durable_lsn();

        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));

            LSN new_durable_lsn = wal_manager_.get_durable_lsn();
            if (last_durable_lsn != new_durable_lsn)
            {
                flush(new_durable_lsn);
                last_durable_lsn = new_durable_lsn;
            }
        } while (!stop_bg_thread_);
    }

    void
    FlushCoordinator::flush()
    {
        catalog_.flush();
        buffer_pool_.flush_dirty();
    }

    void
    FlushCoordinator::flush(LSN max_lsn)
    {
        catalog_.flush(max_lsn);
        buffer_pool_.flush_dirty(max_lsn);
    }
}