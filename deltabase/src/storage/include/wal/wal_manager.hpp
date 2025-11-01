#pragma once

#include "../objects/meta_object.hpp"
#include "../objects/data_object.hpp"
#include "../file_manager.hpp"
#include "wal_object.hpp"
#include "wal_writer.hpp"
#include <chrono>
#include <thread>

namespace storage {
    class WalManager {
        FileManager& fm_;
        std::string db_name_;

        std::vector<WalLogfile> log_;
        std::vector<WalRecord> buffer_;
        std::mutex buffer_mtx_;

        std::thread bg_writer_;
        WalWriter writer_;

        std::chrono::duration<int64_t> checkpoint_interval_ = std::chrono::minutes(1);

        void
        load_log();

        WalLogfile*
        last_logfile();

        WalLogfile
        create_logfile();

        void
        insert_to_buffer(const WalRecord& record);

        // прочитать
        // потом слить на диск
        // как сливать: функция слива на диск, будет вызываться из checkpoint_ctl
    public:
        WalManager(std::string db_name, FileManager& fm);

        void
        push_create_schema(const MetaSchema& schema);
        void
        push_drop_schema(const MetaSchema& schema);
        void
        push_create_table(const MetaTable& table);

        void 
        push_insert(MetaTable& table, const DataRow& row);

        void 
        write_update_by_filter();

        void 
        write_delete_by_filter();

        void
        flush_on_disk();
    };
}