//
// Created by poproshaikin on 10/1/26.
//

#ifndef DELTABASE_DB_LOCK_HPP
#define DELTABASE_DB_LOCK_HPP
#include <filesystem>
#ifdef _WIN32
  #include <windows.h>
#endif

namespace storage
{
    class DbLock
    {
#ifdef _WIN32
        HANDLE h_;
#else
        int fd_;
#endif
    public:
        DbLock(const std::filesystem::path& db_path, const std::string& db_name);
        ~DbLock();

        DbLock(const DbLock&) = delete;
        DbLock& operator=(const DbLock&) = delete;
    };
}

#endif //DELTABASE_DB_LOCK_HPP