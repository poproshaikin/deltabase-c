#include "checkpoint_manager.hpp"

#include "crc32.hpp"
#include "logger.hpp"
#include "transaction_manager.hpp"

#include <ranges>

//
// Created by poproshaikin on 9/24/26.
//
namespace recovery
{
    using namespace types;
    using namespace wal;
    using namespace txn;
    using namespace storage;

    CheckpointManager::CheckpointManager(
        IWALManager& wal,
        IIOManager& io,
        BufferPool& buffer_pool,
        TransactionManager& txn_manager)
        : running_(false), wal_(wal), io_(io), buffer_pool_(buffer_pool), txn_manager_(txn_manager)
    {
    }

    CheckpointManager::~CheckpointManager()
    {
        stop_background();
    }

    void
    CheckpointManager::start_background(std::chrono::milliseconds interval)
    {
        running_ = true;
        bg_thread_ = std::thread([this, interval]
        {
            std::unique_lock lock(cv_mtx_);
            while (running_)
            {
                if (cv_.wait_for(lock, interval, [this] { return !running_.load(); }))
                    break;

                lock.unlock();
                try
                {
                    do_checkpoint();
                }
                catch (const std::exception& e)
                {
                    misc::Logger::error(std::string("CheckpointManager: checkpoint failed: ") + e.what());
                }
                lock.lock();
            }
        });
    }

    void
    CheckpointManager::stop_background()
    {
        running_ = false;
        cv_.notify_all();

        if (bg_thread_.joinable())
            bg_thread_.join();
    }

    static LSN
    compute_redo_lsn(const std::vector<std::pair<UUID, LSN>>& dpt, LSN begin_lsn)
    {
        if (dpt.empty())
            return begin_lsn;

        LSN min_rec_lsn = dpt.front().second;
        for (const auto& rec_lsn : dpt | std::views::values)
            min_rec_lsn = std::min(min_rec_lsn, rec_lsn);

        return min_rec_lsn;
    }

    void
    CheckpointManager::do_checkpoint()
    {
        std::lock_guard lock(checkpoint_mtx_);

        LSN begin_lsn = wal_.append_log(BeginCkptRecord());

        auto dpt_snapshot = buffer_pool_.snapshot_dpt();
        auto att_snapshot = txn_manager_.snapshot_att();

        auto redo_lsn = compute_redo_lsn(dpt_snapshot, begin_lsn);

        EndCkptRecord record(
            begin_lsn,
            redo_lsn,
            att_snapshot,
            dpt_snapshot);

        LSN end_lsn = wal_.append_log(record);

        wal_.ensure_durable(end_lsn);

        io_.write_control_file(
            {redo_lsn, misc::crc32(reinterpret_cast<const uint8_t*>(&redo_lsn), sizeof(redo_lsn))});
    }
}