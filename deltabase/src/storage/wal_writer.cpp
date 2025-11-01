#include "include/wal/wal_writer.hpp"
#include "include/paths.hpp"

#include <fstream>
#include <sys/stat.h>
#include <cstdint>
#include <filesystem>
#include <variant>
#include <iostream>

namespace storage {
    // std::fstream
    // wal_writer::get_available_logfile() {
    //     auto dir_path = path_db_wal();
    //     for (const auto& entry : fs::directory_iterator(dir_path)) {
    //         if (!entry.is_regular_file()) 
    //             continue;

                
    //         if (stat(entry.path().c_str(), &st) != 0) {
    //             std::cerr << "Warning: in wal_writer::get_available_logfile: failed to get stat of the file " + entry.path().string();
    //             continue;
    //         }

    //         if (st.st_size < max_logfile_size) 
    //             return std::fstream(entry.path());
    //     }

    //     return create_logfile();
    // }

    // void
    // wal_writer::write_batch(const std::vector<wal_record>& wal) {
    //     // найти подходящий файл
    //     // пока его размер не выге максимума, записывать record в stream
    //     auto stream = get_available_logfile();

    //     for (const auto& record : wal) {
    //         std::visit([this, &stream](const auto& record) {
    //             uint64_t size = record.estimate_size();   
    //             std::streamoff filesize = stream.tellg();

    //             if (filesize + size > wal_writer::max_logfile_size) {
    //                 stream.close();
    //                 stream = get_available_logfile();
    //             }

    //             return write_record(record, stream); 
    //         }, record);
    //     }
    // }

    // void 
    // wal_writer::write_record(const wal_record& record, wal_logfile& fs) {
    //     std::visit([this, &fs](auto rec) {
    //         auto serialized = rec.serialize();
    //         fs.write(reinterpret_cast<const char*>(serialized.data()), serialized.size());
    //     }, record);
    // }
}