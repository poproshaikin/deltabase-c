//
// Created by poproshaikin on 13.11.25.
//

#include "memory_stream.hpp"

#include "utils.hpp"
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
        size_t n = std::min(count, other.buffer_.size());
        buffer_.insert(buffer_.end(), other.buffer_.begin(), other.buffer_.begin() + n);
        position_ += n;
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

    void
    MemoryStream::write_u64(uint64_t value, bool big_endian)
    {
        uint64_t effective = big_endian ? to_big_endian_u64(value) : value;
        write(&effective, sizeof(effective));
    }

    void
    MemoryStream::write_message_type(types::NetMessageType type)
    {
        const auto raw_type = static_cast<uint8_t>(type);
        write(&raw_type, sizeof(raw_type));
    }

    void
    MemoryStream::write_uuid(const types::UUID& session_id)
    {
        write(session_id.raw(), sizeof(*session_id.raw()));
    }

    void
    MemoryStream::write_string(const std::string& value, bool big_endian)
    {
        const auto size = big_endian ? to_big_endian_u64(value.size()) : value.size();
        write(&size, sizeof(size));
        if (!value.empty())
        {
            write(value.data(), value.size());
        }
    }

    void
    MemoryStream::write_bytes(const types::Bytes& bytes, bool big_endian)
    {
        const auto size = big_endian ? to_big_endian_u64(bytes.size()) : bytes.size();
        write(&size, sizeof(size));
        if (!bytes.empty())
        {
            write(bytes.data(), bytes.size());
        }
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

    bool
    ReadOnlyMemoryStream::read_exact(void* out, uint64_t size)
    {
        return read(out, size) == size;
    }

    bool
    ReadOnlyMemoryStream::read_u64(uint64_t& out, bool big_endian)
    {
        uint64_t effective = big_endian ? to_big_endian_u64(out) : out;
        return read(&effective, sizeof(effective)) == sizeof(out);
    }

    bool
    ReadOnlyMemoryStream::read_string(std::string& out, bool big_endian)
    {
        uint64_t size_be = 0;
        if (!read_exact(&size_be, sizeof(size_be)))
        {
            return false;
        }

        const auto size = big_endian ? from_big_endian_u64(size_be) : size_be;
        if (size > remaining())
        {
            return false;
        }

        out.resize(size);

        if (size == 0)
        {
            return true;
        }

        return read_exact(out.data(), size);
    }

    bool
    ReadOnlyMemoryStream::read_bytes(types::Bytes& out, bool big_endian)
    {
        uint64_t size_be = 0;
        if (!read_exact(&size_be, sizeof(size_be)))
        {
            return false;
        }

        const auto size = big_endian ? from_big_endian_u64(size_be) : size_be;
        if (size > remaining())
        {
            return false;
        }

        out.resize(size);

        if (size == 0)
        {
            return true;
        }

        return read_exact(out.data(), size);
    }

    bool
    ReadOnlyMemoryStream::read_uuid(types::UUID& out)
    {
        uuid_t raw{};
        if (!read_exact(&raw, sizeof(raw)))
        {
            return false;
        }

        out = types::UUID(raw);
        return true;
    }
}