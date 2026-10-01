//
// Created by poproshaikin on 9/29/26.
//

#include "page_file_lock.hpp"

#include <stdexcept>
#include <unistd.h>
#include <sys/file.h>

namespace storage
{
    PageFileLock::PageFileLock(const std::string& path)
    {
#ifdef _WIN32
        handle_ = CreateFileW(
            path.wstring().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle_ = INVALID_HANDLE_VALUE)
            throw std::runtime_error("PAgeFileLock: CreateFile failed");

        OVERLAPPED ov{};
        if (!LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ov))
            throw std::runtime_error("PAgeFileLock: LockFileEx failed");
#else
        fd_ = open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd_ < 0)
            throw std::runtime_error("PAgeFileLock: open failed");
        if (flock(fd_, LOCK_EX) < 0)
            throw std::runtime_error("PAgeFileLock: flock failed");
#endif
    }

    PageFileLock::~PageFileLock()
    {
#ifdef _WIN32
        OVERLAPPED ov{};
        UnlockFileEx(handle_, 0, MAXDWORD, MAXDWORD, &ov);
        CloseHandle(handle_);
        handle_ = INVALID_HANDLE_VALUE;
#else
        flock(fd_, LOCK_UN);
        close(fd_);
        fd_ = 0;
#endif

    }
}
