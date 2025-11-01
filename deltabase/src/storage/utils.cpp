#include "include/utils.hpp"
#include <cstdint>
#include <iostream>

namespace storage {
    uint64_t
    get_file_size(const std::filesystem::path &path) {
        try {
            return std::filesystem::file_size(path);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << '\n';
            return 0;
        }
    }
}