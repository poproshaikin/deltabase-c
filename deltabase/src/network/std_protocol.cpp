//
// Created by poproshaikin on 4/13/26.
//

#include "include/std_protocol.hpp"

#include "memory_stream.hpp"

#include <cstdint>

namespace net
{
    using namespace misc;

    namespace
    {
        uint64_t
        to_big_endian_u64(uint64_t value)
        {
            return ((value & 0x00000000000000FFULL) << 56)
                | ((value & 0x000000000000FF00ULL) << 40)
                | ((value & 0x0000000000FF0000ULL) << 24)
                | ((value & 0x00000000FF000000ULL) << 8)
                | ((value & 0x000000FF00000000ULL) >> 8)
                | ((value & 0x0000FF0000000000ULL) >> 24)
                | ((value & 0x00FF000000000000ULL) >> 40)
                | ((value & 0xFF00000000000000ULL) >> 56);
        }

        uint64_t
        from_big_endian_u64(uint64_t value)
        {
            return to_big_endian_u64(value);
        }

        void
        write_message_type(MemoryStream& stream, types::NetMessageType type)
        {
            const auto raw_type = static_cast<uint8_t>(type);
            stream.write(&raw_type, sizeof(raw_type));
        }

        void
        write_uuid(MemoryStream& stream, const types::UUID& session_id)
        {
            stream.write(session_id.raw(), sizeof(*session_id.raw()));
        }

        void
        write_string(MemoryStream& stream, const std::string& value)
        {
            const auto size = static_cast<uint64_t>(value.size());
            const auto size_be = to_big_endian_u64(size);
            stream.write(&size_be, sizeof(size_be));
            if (!value.empty())
            {
                stream.write(value.data(), value.size());
            }
        }

        bool
        read_exact(ReadOnlyMemoryStream& stream, void* out, uint64_t size)
        {
            return stream.read(out, size) == size;
        }

        bool
        read_string(ReadOnlyMemoryStream& stream, std::string& out)
        {
            uint64_t size_be = 0;
            if (!read_exact(stream, &size_be, sizeof(size_be)))
            {
                return false;
            }

            const auto size = from_big_endian_u64(size_be);
            if (size > stream.remaining())
            {
                return false;
            }

            out.resize(size);

            if (size == 0)
            {
                return true;
            }

            return read_exact(stream, out.data(), size);
        }

        bool
        read_uuid(ReadOnlyMemoryStream& stream, types::UUID& out)
        {
            uuid_t raw{};
            if (!read_exact(stream, &raw, sizeof(raw)))
            {
                return false;
            }

            out = types::UUID(raw);
            return true;
        }

        types::NetMessage
        protocol_violation_message()
        {
            return types::CloseNetMessage(types::UUID::null());
        }
    } // namespace

    types::Bytes
    StdNetProtocol::encode(const types::PingNetMessage& msg) const
    {
        MemoryStream stream;
        write_message_type(stream, msg.type);
        return stream.to_vector();
    }

    types::Bytes
    StdNetProtocol::encode(const types::PongNetMessage& msg) const
    {
        MemoryStream stream;
        write_message_type(stream, msg.type);
        write_uuid(stream, msg.session_id);

        const auto raw_err = static_cast<uint8_t>(msg.err);
        stream.write(&raw_err, sizeof(raw_err));

        return stream.to_vector();
    }

    types::Bytes
    StdNetProtocol::encode(const types::QueryNetMessage& msg) const
    {
        MemoryStream stream;
        write_message_type(stream, msg.type);
        write_uuid(stream, msg.session_id);
        write_string(stream, msg.query);
        return stream.to_vector();
    }

    types::Bytes
    StdNetProtocol::encode(const types::CreateDbNetMessage& msg) const
    {
        MemoryStream stream;
        write_message_type(stream, msg.type);
        write_uuid(stream, msg.session_id);
        write_string(stream, msg.db_name);

        return stream.to_vector();
    }

    types::Bytes
    StdNetProtocol::encode(const types::AttachDbNetMessage& msg) const
    {
        MemoryStream stream;
        write_message_type(stream, msg.type);
        write_uuid(stream, msg.session_id);
        write_string(stream, msg.db_name);

        return stream.to_vector();
    }

    types::Bytes
    StdNetProtocol::encode(const types::CloseNetMessage& msg) const
    {
        MemoryStream stream;
        write_message_type(stream, msg.type);
        write_uuid(stream, msg.session_id);
        return stream.to_vector();
    }


    types::Bytes
    StdNetProtocol::encode(const types::NetMessage& msg) const
    {
        return std::visit([this](const auto& val)
        {
            return encode(val);
        }, msg);
    }

    types::NetMessage
    StdNetProtocol::parse(const types::Bytes& data) const
    {
        ReadOnlyMemoryStream stream(data);

        uint8_t raw_type = 0;
        if (!read_exact(stream, &raw_type, sizeof(raw_type)))
        {
            return protocol_violation_message();
        }

        const auto type = static_cast<types::NetMessageType>(raw_type);

        switch (type)
        {
        case types::NetMessageType::PING:
            return types::PingNetMessage();

        case types::NetMessageType::PONG:
        {
            types::UUID session_id;
            if (!read_uuid(stream, session_id))
            {
                return protocol_violation_message();
            }

            uint8_t raw_err = 0;
            if (!read_exact(stream, &raw_err, sizeof(raw_err)))
            {
                return protocol_violation_message();
            }

            return types::PongNetMessage(session_id, static_cast<types::NetErrorCode>(raw_err));
        }

        case types::NetMessageType::QUERY:
        {
            types::UUID session_id;
            if (!read_uuid(stream, session_id))
            {
                return protocol_violation_message();
            }

            std::string query;
            if (!read_string(stream, query))
            {
                return protocol_violation_message();
            }

            return types::QueryNetMessage(session_id, query);
        }

        case types::NetMessageType::CREATE_DB:
        {
            types::UUID session_id;
            if (!read_uuid(stream, session_id))
            {
                return protocol_violation_message();
            }

            std::string db_name;
            if (!read_string(stream, db_name))
            {
                return protocol_violation_message();
            }

            return types::CreateDbNetMessage(session_id, db_name);
        }

        case types::NetMessageType::ATTACH_DB:
        {
            types::UUID session_id;
            if (!read_uuid(stream, session_id))
            {
                return protocol_violation_message();
            }

            std::string db_name;
            if (!read_string(stream, db_name))
            {
                return protocol_violation_message();
            }

            return types::AttachDbNetMessage(session_id, db_name);
        }

        case types::NetMessageType::CLOSE:
        {
            types::UUID session_id;
            if (!read_uuid(stream, session_id))
            {
                return protocol_violation_message();
            }

            return types::CloseNetMessage(session_id);
        }

        case types::NetMessageType::UNDEFINED:
            return protocol_violation_message();
        }

        return protocol_violation_message();
    }
} // namespace net
