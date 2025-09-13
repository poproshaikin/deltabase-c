#pragma once 

#include <memory>
#include <vector>

namespace storage {
    class FileDeleter {
        void
        operator()(void* ptr) {
            std::free(ptr);
        }
    };

    using unique_void_ptr = std::unique_ptr<void, FileDeleter>;
    using bytes_arr = std::vector<uint8_t>;
}