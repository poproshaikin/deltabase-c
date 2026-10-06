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

    // same as write_file, but takes no flock - only safe when the caller
    // already has exclusive access to the db guaranteed some other way
    // (today: BufferPool::write_nolock, see its callers)
    void
    write_file_nolock(const fs::path& path, const types::Bytes& content);

    void
    append_file(const fs::path& path, const types::Bytes& content);

    bool
    exists_file(const fs::path& path);

    void
    fsync_file(const fs::path& path, const types::Bytes& content);

    void
    fsync_file(const fs::path& path);

    // same as fsync_file, but takes no flock - see write_file_nolock
    void
    fsync_file_nolock(const fs::path& path, const types::Bytes& content);
}

#endif //DELTABASE_UTILS_HPP