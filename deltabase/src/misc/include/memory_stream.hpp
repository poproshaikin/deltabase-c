//
// Created by poproshaikin on 13.11.25.
//

#ifndef DELTABASE_MEMORY_STREAM_HPP
#define DELTABASE_MEMORY_STREAM_HPP
#include "utils.hpp"
#include "../../types/include/typedefs.hpp"
#include "../../types/include/net_message.hpp"

#include <cstdint>
#include <vector>

namespace misc
{
    class MemoryStream
    {
        std::vector<uint8_t> buffer_;
        size_t position_ = 0;

    public:
        MemoryStream() = default;

        explicit
        MemoryStream(const std::vector<uint8_t>& buffer);

        size_t
        read(void* dest, size_t count);

        void
        write(const void* src, size_t count);

        void
        append(MemoryStream& other, size_t count);

        const uint8_t*
        data() const noexcept;

        uint8_t*
        data() noexcept;

        void
        seek(size_t pos);

        size_t
        tell() const;

        types::Bytes
        to_vector() const;

        size_t
        size() const;

        void
        write_u64(uint64_t value, bool big_endian);

        void
        write_message_type(types::NetMessageType type);

        void
        write_uuid(const types::UUID& session_id);

        void
        write_string(const std::string& value, bool big_endian);

        void
        write_bytes(const types::Bytes& bytes, bool big_endian);

    };

    class ReadOnlyMemoryStream
    {
        const std::vector<uint8_t> buffer_;
        uint64_t position_ = 0;

    public:
        explicit
        ReadOnlyMemoryStream(const std::vector<uint8_t>& buffer);

        uint64_t
        read(void* dest, uint64_t count);

        uint64_t
        read(std::string& out);

        void
        seek(size_t pos);

        uint64_t
        tell() const;

        uint64_t
        size() const;

        size_t
        remaining() const;

        std::vector<uint8_t>
        to_vector() const;

        const uint8_t*
        current() const noexcept;

        bool
        read_exact(void* out, uint64_t size);

        bool
        read_u64(uint64_t& out, bool big_endian);

        bool
        read_string(std::string& out, bool big_endian);

        bool
        read_bytes(types::Bytes& out, bool big_endian);

        bool
        read_uuid(types::UUID& out);
    };
}

#endif //DELTABASE_MEMORY_STREAM_HPP