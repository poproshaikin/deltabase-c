#pragma once 

#include <stdint.h>
#include <filesystem>

namespace storage {
    uint64_t
    get_file_size(const std::filesystem::path& path);
}