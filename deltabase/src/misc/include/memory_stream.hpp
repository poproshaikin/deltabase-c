//
// Created by poproshaikin on 13.11.25.
//

#ifndef DELTABASE_MEMORY_STREAM_HPP
#define DELTABASE_MEMORY_STREAM_HPP
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace misc
{
    class MemoryStream
    {
        std::vector<uint8_t>& buffer_;
        mutable size_t position_;

    public:
        explicit
        MemoryStream(std::vector<uint8_t>& buffer);

        size_t
        read(void* dest, size_t count) const;

        void
        write(const void* src, size_t count) const;

        void
        seek(size_t pos) const;

        size_t
        tell() const;
    };

    class ReadOnlyMemoryStream
    {
        const std::vector<uint8_t>& buffer_;
        mutable uint64_t position_;

    public:
        explicit
        ReadOnlyMemoryStream(const std::vector<uint8_t>& buffer);

        uint64_t
        read(void* dest, uint64_t count) const;

        uint64_t
        read(std::string& out) const;

        void
        seek(size_t pos) const;

        uint64_t
        tell() const;

        uint64_t
        size() const;

        size_t
        remaining() const;
    };
}

#endif //DELTABASE_MEMORY_STREAM_HPP