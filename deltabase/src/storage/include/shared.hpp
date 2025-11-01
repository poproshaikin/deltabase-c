#pragma once 

#include <memory>
#include <vector>

namespace storage {
    class file_deleter {
        void
        operator()(void* ptr) {
            std::free(ptr);
        }
    };

    using unique_void_ptr = std::unique_ptr<void, file_deleter>;
    using bytes_v = std::vector<uint8_t>;
}