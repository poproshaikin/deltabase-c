#pragma once

namespace storage {
    class checkpoint_ctl {
    public:
        void
        flush_wal();
    };
}