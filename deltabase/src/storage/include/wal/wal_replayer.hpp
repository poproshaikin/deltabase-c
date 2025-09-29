#pragma once 

namespace storage {
    class wal_replayer {
        void
        replay();
        
    public:
        void
        run_bg_worker();
    };
}