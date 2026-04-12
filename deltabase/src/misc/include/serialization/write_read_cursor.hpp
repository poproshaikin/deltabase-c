#ifndef DELTABASE_WRITE_READ_CURSOR_HPP
#define DELTABASE_WRITE_READ_CURSOR_HPP

#include "../../../types/include/UUID.hpp"
#include "../memory_stream.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

namespace misc::serialization
{
    class WriteCursor
    {
        MemoryStream& stream_;

    public:
        explicit WriteCursor(MemoryStream& stream) : stream_(stream)
        {
        }

        MemoryStream&
        stream()
        {
            return stream_;
        }

        bool
        write_bytes(const void* data, size_t size)
        {
            stream_.write(data, size);
            return true;
        }

        template <typename T>
        bool
        write_pod(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>, "write_pod requires trivially copyable type");
            stream_.write(&value, sizeof(T));
            return true;
        }

        bool
        write_uuid(const types::UUID& value)
        {
            return write_bytes(value.raw(), sizeof(uuid_t));
        }

        bool
        write_string(const std::string& value)
        {
            uint64_t size = value.size();
            stream_.write(&size, sizeof(size));
            stream_.write(value.data(), size);
            return true;
        }
    };

    class ReadCursor
    {
        ReadOnlyMemoryStream& stream_;

    public:
        explicit ReadCursor(ReadOnlyMemoryStream& stream) : stream_(stream)
        {
        }

        ReadOnlyMemoryStream&
        stream()
        {
            return stream_;
        }

        bool
        read_bytes(void* out, uint64_t size)
        {
            return stream_.read(out, size) == size;
        }

        template <typename T>
        bool
        read_pod(T& out)
        {
            static_assert(std::is_trivially_copyable_v<T>, "read_pod requires trivially copyable type");
            return stream_.read(&out, sizeof(T)) == sizeof(T);
        }

        bool
        read_uuid(types::UUID& out)
        {
            return read_bytes(out.raw(), sizeof(uuid_t));
        }

        bool
        read_string(std::string& out)
        {
            uint64_t size = 0;
            if (!read_pod(size))
                return false;

            out.resize(size);
            return read_bytes(out.data(), size);
        }
    };
} // namespace misc::serialization

#endif // DELTABASE_WRITE_READ_CURSOR_HPP
