//
// Created by poproshaikin on 13.11.25.
//

#include "memory_stream.hpp"

#include "../types/include/typedefs.hpp"

#include <cstring>

namespace misc
{
    MemoryStream::MemoryStream(const types::Bytes& buffer) : buffer_(buffer)
    {
    }

    size_t
    MemoryStream::read(void* dest, size_t count)
    {
        if (position_ + count > buffer_.size())
            count = buffer_.size() - position_;

        std::memcpy(dest, buffer_.data() + position_, count);
        position_ += count;
        return count;
    }

    void
    MemoryStream::write(const void* src, size_t count)
    {
        if (position_ + count > buffer_.size())
            buffer_.resize(position_ + count);

        std::memcpy(
            buffer_.data() + position_,
            src,
            count
        );
        position_ += count;
    }

    void
    MemoryStream::append(MemoryStream& other, size_t count)
    {
        buffer_.insert(buffer_.end(), other.buffer_.begin(), other.buffer_.end());
    }

    const uint8_t*
    MemoryStream::data() const noexcept
    {
        return buffer_.data();
    }

    uint8_t*
    MemoryStream::data() noexcept
    {
        return buffer_.data();
    }

    void
    MemoryStream::seek(size_t pos)
    {
        if (pos > buffer_.size())
            throw std::out_of_range("seek past end");

        position_ = pos;
    }

    size_t
    MemoryStream::tell() const
    {
        return position_;
    }

    types::Bytes
    MemoryStream::to_vector() const
    {
        return buffer_;
    }

    size_t
    MemoryStream::size() const
    {
        return buffer_.size();
    }

    ReadOnlyMemoryStream::ReadOnlyMemoryStream(
        const std::vector<uint8_t>& buffer) : buffer_(buffer)
    {
    }

    uint64_t
    ReadOnlyMemoryStream::read(void* dest, uint64_t count)
    {
        if (position_ + count > buffer_.size())
            count = buffer_.size() - position_;

        std::memcpy(dest, buffer_.data() + position_, count);
        position_ += count;
        return count;
    }

    uint64_t
    ReadOnlyMemoryStream::read(std::string& out)
    {
        uint64_t len = 0;
        if (read(&len, sizeof(uint64_t)) != sizeof(uint64_t))
            return 0;

        out.resize(len);
        if (read(&out[0], len) != len)
            return 0;

        return len + sizeof(uint64_t);
    }

    void
    ReadOnlyMemoryStream::seek(size_t pos)
    {
        if (pos > buffer_.size())
            throw std::out_of_range("seek past end");

        position_ = pos;
    }

    uint64_t
    ReadOnlyMemoryStream::tell() const
    {
        return position_;
    }

    uint64_t
    ReadOnlyMemoryStream::size() const
    {
        return buffer_.size();
    }

    size_t
    ReadOnlyMemoryStream::remaining() const
    {
        return buffer_.size() - position_;
    }

    std::vector<uint8_t>
    ReadOnlyMemoryStream::to_vector() const
    {
        return buffer_;
    }

    const uint8_t*
    ReadOnlyMemoryStream::current() const noexcept
    {
        return buffer_.data() + position_;
    }
}