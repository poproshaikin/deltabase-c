#include "include/wal/wal_writer.hpp"
#include "include/paths.hpp"
#include <variant>

namespace storage {
    std::fstream
    WalWriter::get_available_logfile() {
        auto dir_path = path_db_wal();
        return std::fstream(dir_path, std::ios::binary | std::ios::out | std::ios::app);
    }

    void
    WalWriter::write_batch(const std::vector<WalRecord>& wal) {
        auto stream = get_available_logfile();

        for (const auto& record : wal) {
            std::visit([this, &stream](const auto& record) {
                return write_record(record, stream); 
            }, record);
        }
    }

    void 
    WalWriter::write_record(const WalRecord& record, std::fstream& fs) {
        std::visit([this, &fs](auto rec) {
            auto serialized = rec.serialize();
            fs.write(reinterpret_cast<const char*>(serialized.data()), serialized.size());
        }, record);
    }
}