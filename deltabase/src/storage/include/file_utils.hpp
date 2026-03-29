//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_UTILS_HPP
#define DELTABASE_UTILS_HPP
#include "typedefs.hpp"

namespace storage
{
    namespace fs = std::filesystem;

    types::Bytes
    read_file(const fs::path& path);

    void
    write_file(const fs::path& path, const types::Bytes& content);

    void
    fsync_file(const fs::path& path, const types::Bytes& content);
}

#endif //DELTABASE_UTILS_HPP
