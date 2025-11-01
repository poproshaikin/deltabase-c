#pragma once

#include "../pages/page.hpp"
#include "wal_object.hpp"
#include <fstream>

namespace storage {
    class WalWriter {
        static constexpr uint64_t max_logfile_size = DataPage::max_size * 256; 


        // std::fstream
        // get_available_logfile();
    public:
        void 
        write_record(const WalRecord& record, WalLogfile& fstream);

        // void 
        // write_batch(const std::vector<wal_record>& batch);
    };
}