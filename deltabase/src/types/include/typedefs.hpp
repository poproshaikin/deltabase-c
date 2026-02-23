//
// Created by poproshaikin on 09.11.25.
//

#ifndef DELTABASE_TYPEDEFS_HPP
#define DELTABASE_TYPEDEFS_HPP
#include <cstdint>
#include <vector>
#include <filesystem>

namespace types
{
    namespace fs = std::filesystem;
    using Bytes = std::vector<uint8_t>;
    using RowId = uint64_t;
}

#endif //DELTABASE_TYPEDEFS_HPP