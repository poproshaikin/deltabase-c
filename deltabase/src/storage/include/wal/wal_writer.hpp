#pragma once

#include "wal_object.hpp"
#include <fstream>

namespace storage {
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