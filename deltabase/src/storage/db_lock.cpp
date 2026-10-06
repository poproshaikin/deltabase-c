//
// Created by poproshaikin on 10/1/26.
//

#include "db_lock.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include "path.hpp"
#include "../misc/include/exceptions.hpp"

namespace storage
{
    DbLock::DbLock(const std::filesystem::path& db_path, const std::string& db_name)
    {
        auto path = path_db_lock(db_path, db_name);
#ifdef _WIN32
        h_ = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (h_ == INVALID_HANDLE_VALUE)
            throw EngineException(
                "DbLock: failed to open lock file for database " + db_name,
                EngineException::Code::DB_LOCKED);

        OVERLAPPED ov{};
        if (!LockFileEx(h_, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, MAXDWORD, MAXDWORD, &ov))
        {
            CloseHandle(h_);
            throw EngineException(
                "DbLock: database " + db_name + " is already attached by another process/session",
                EngineException::Code::DB_LOCKED);
        }
#else
        fd_ = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd_ < 0)
            throw EngineException(
                "DbLock: failed to open lock file for database " + db_name,
                EngineException::Code::DB_LOCKED);

        if (::flock(fd_, LOCK_EX | LOCK_NB) < 0)
        {
            ::close(fd_);
            throw EngineException(
                "DbLock: database " + db_name + " is already attached by another process/session",
                EngineException::Code::DB_LOCKED);
        }
#endif
    }

    DbLock::~DbLock()
    {
#ifdef _WIN32
        OVERLAPPED ov{};
        UnlockFileEx(h_, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle(h_);
#else
        ::flock(fd_, LOCK_UN);
        ::close(fd_);
#endif
    }
}