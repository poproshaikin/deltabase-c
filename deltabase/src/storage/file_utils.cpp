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
#include <sys/file.h>
#include <sys/stat.h>
#endif

namespace storage
{
    using namespace types;

    Bytes
    read_file(const fs::path& path)
    {
#ifdef _WIN32
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file: " + path.string());

        std::streamsize size = file.tellg(); // file size
        file.seekg(0, std::ios::beg); // return to start

        std::vector<uint8_t> buffer(size);

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
            throw std::runtime_error("Error reading file: " + path.string());

        return buffer;
#else
        const int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0)
            throw std::runtime_error("Cannot open file: " + path.string());

        if (flock(fd, LOCK_SH) < 0)
        {
            close(fd);
            throw std::runtime_error("Cannot acquire shared lock for file: " + path.string());
        }

        struct stat st;
        if (fstat(fd, &st) < 0)
        {
            flock(fd, LOCK_UN);
            close(fd);
            throw std::runtime_error("Cannot stat file: " + path.string());
        }

        const auto size = static_cast<size_t>(st.st_size);
        std::vector<uint8_t> buffer(size);

        size_t total = 0;
        while (total < size)
        {
            const auto read_bytes = read(fd, buffer.data() + total, size - total);
            if (read_bytes < 0)
            {
                flock(fd, LOCK_UN);
                close(fd);
                throw std::runtime_error("Error reading file: " + path.string());
            }

            if (read_bytes == 0)
                break;

            total += static_cast<size_t>(read_bytes);
        }

        buffer.resize(total);
        flock(fd, LOCK_UN);
        close(fd);

        return buffer;
#endif
    }

    void
    write_file(const fs::path& path, const Bytes& content)
    {
        if (!fs::exists(path.parent_path()))
            fs::create_directories(path.parent_path());

#ifdef _WIN32
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        file.close();
#else
        const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            throw std::runtime_error("Cannot open file for writing: " + path.string());

        if (flock(fd, LOCK_EX) < 0)
        {
            close(fd);
            throw std::runtime_error("Cannot acquire exclusive lock for file: " + path.string());
        }

        size_t total = 0;
        while (total < content.size())
        {
            const auto written = write(fd, content.data() + total, content.size() - total);
            if (written < 0)
            {
                flock(fd, LOCK_UN);
                close(fd);
                throw std::runtime_error("Error writing file: " + path.string());
            }

            total += static_cast<size_t>(written);
        }

        flock(fd, LOCK_UN);
        close(fd);
#endif
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

        if (flock(fd, LOCK_EX) < 0)
        {
            close(fd);
            throw std::runtime_error("fsync_file: flock failed");
        }

        ssize_t total = 0;
        const uint8_t* data = content.data();
        ssize_t size = content.size();

        while (total < size)
        {
            ssize_t written = write(fd, data + total, size - total);
            if (written < 0)
            {
                flock(fd, LOCK_UN);
                close(fd);
                throw std::runtime_error("fsync_file: write failed");
            }
            total += written;
        }

        if (fsync(fd) < 0)
        {
            flock(fd, LOCK_UN);
            close(fd);
            throw std::runtime_error("fsync_file: fsync failed");
        }

        flock(fd, LOCK_UN);
        close(fd);
#endif
    }

} // namespace storage