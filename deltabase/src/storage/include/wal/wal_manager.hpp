#pragma once

#include "../objects/meta_object.hpp"
#include "../objects/data_object.hpp"
#include "wal_record.hpp"
#include "wal_writer.hpp"
#include <chrono>
#include <thread>

namespace storage {
    class WalManager {
        std::vector<WalRecord> log_;

        std::vector<WalRecord> buffer_;
        std::mutex buffer_mtx_;

        std::thread bg_writer_;
        WalWriter writer_;

        std::chrono::duration<uint64_t> flush_interval_ = std::chrono::seconds(1);

        void 
        run_bg_writer();

        void
        insert_to_buffer(const WalRecord& record);

    public:
        void
        push_create_schema();
        
        void 
        write_create_table();

        void 
        push_insert(MetaTable& table, const DataRow& row);

        void 
        write_update_by_filter();

        void 
        write_delete_by_filter();
    };
}