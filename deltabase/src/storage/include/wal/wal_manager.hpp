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

        std::vector<wal_logfile> log_;
        std::vector<wal_record> buffer_;
        std::mutex buffer_mtx_;

        std::thread bg_writer_;
        wal_writer writer_;

        std::chrono::duration<int64_t> checkpoint_interval_ = std::chrono::minutes(1);

        void
        load_log();

        wal_logfile*
        last_logfile();

        wal_logfile
        create_logfile();

        void
        insert_to_buffer(const wal_record& record);

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