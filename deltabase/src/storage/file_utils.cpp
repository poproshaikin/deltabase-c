//
// Created by poproshaikin on 15.01.26.
//

#include "file_utils.hpp"

#include <fstream>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif

namespace storage
{
    using namespace types;

    Bytes
    read_file(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file: " + std::string(path));

        std::streamsize size = file.tellg(); // file size
        file.seekg(0, std::ios::beg); // return to start

        std::vector<uint8_t> buffer(size);

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
            throw std::runtime_error("Error reading file: " + std::string(path));

        return buffer;
    }

    void
    write_file(const fs::path& path, const Bytes& content)
    {
        if (!fs::exists(path.parent_path()))
            fs::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        file.close();
    }

    bool
    exists_file(const fs::path& path)
    {
        return std::filesystem::exists(path) &&
               std::filesystem::is_regular_file(path);
    }

    void
    fsync_file(const fs::path& path, const Bytes& content)
    {
        fs::create_directories(path.parent_path());
#ifdef _WIN32
        // --- Windows ---
        HANDLE file = CreateFileW(
            path.wstring().c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (file == INVALID_HANDLE_VALUE)
            throw std::runtime_error("CreateFile failed");

        DWORD written = 0;
        if (!WriteFile(file, content.data(), static_cast<DWORD>(content.size()), &written, nullptr))
        {
            CloseHandle(file);
            throw std::runtime_error("WriteFile failed");
        }

        if (written != content.size())
        {
            CloseHandle(file);
            throw std::runtime_error("Partial write");
        }

        if (!FlushFileBuffers(file))
        {
            CloseHandle(file);
            throw std::runtime_error("FlushFileBuffers failed");
        }

        CloseHandle(file);
#else
        // --- POSIX (Linux, macOS) ---

        int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            throw std::runtime_error("fsync_file: open failed");

        ssize_t total = 0;
        const uint8_t* data = content.data();
        ssize_t size = content.size();

        while (total < size)
        {
            ssize_t written = write(fd, data + total, size - total);
            if (written < 0)
            {
                close(fd);
                throw std::runtime_error("fsync_file: write failed");
            }
            total += written;
        }

        if (fsync(fd) < 0)
        {
            close(fd);
            throw std::runtime_error("fsync_file: fsync failed");
        }

        close(fd);
#endif
    }

} // namespace storage