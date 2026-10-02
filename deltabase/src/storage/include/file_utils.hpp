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

    // same as read_file, but takes no flock - for callers that already hold
    // their own exclusive lock on this path (a locking read here would deadlock)
    types::Bytes
    read_file_nolock(const fs::path& path);

    void
    write_file(const fs::path& path, const types::Bytes& content);

    // same as write_file, but takes no flock - see read_file_nolock
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

    // same as fsync_file, but takes no flock - see read_file_nolock
    void
    fsync_file_nolock(const fs::path& path, const types::Bytes& content);
}

#endif //DELTABASE_UTILS_HPP