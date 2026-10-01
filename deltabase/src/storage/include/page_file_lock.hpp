//
// Created by poproshaikin on 9/29/26.
//

#ifndef DELTABASE_PAGE_FILE_LOCK_HPP
#define DELTABASE_PAGE_FILE_LOCK_HPP
#include <string>

namespace storage
{
    class PageFileLock
    {
    public:
        explicit PageFileLock(const std::string& path);
        ~PageFileLock();

    private:
#ifdef _WIN32
        HANDLE handle_;
#else
        int fd_;
#endif
    };
}

#endif //DELTABASE_PAGE_FILE_LOCK_HPP
