#pragma once

#include "wal_record.hpp"
#include <fstream>
#include <filesystem>

namespace storage {
    namespace fs = std::filesystem;

    class WalWriter {
        void 
        write_record(const WalRecord& record, std::fstream& fstream);

        std::fstream
        get_available_logfile();
    public:
        void 
        write_batch(const std::vector<WalRecord>& batch);
    };
}