//
// Created by poproshaikin on 4/13/26.
//

#include "include/std_protocol.hpp"

#include "memory_stream.hpp"

#include <cstdint>
#include <iostream>

namespace net
{
    using namespace types;
    using namespace misc;

    namespace
    {


        NetMessage
        protocol_violation_message()
        {
            return CloseNetMessage(types::UUID::null());
        }
    } // namespace

    Bytes
    StdNetProtocol::encode(const PingNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const PongNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);

        const auto raw_err = static_cast<uint8_t>(msg.err);
        stream.write(&raw_err, sizeof(raw_err));

        stream.write_bytes(msg.payload, true);

        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const QueryNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write_string(msg.query, true);
        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const CreateDbNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write_string(msg.db_name, true);

        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const AttachDbNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write_string(msg.db_name, true);

        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const CloseNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        return stream.to_vector();
    }


    Bytes
    StdNetProtocol::encode(const NetMessage& msg) const
    {
        return std::visit([this](const auto& val)
        {
            return encode(val);
        }, msg);
    }

    NetMessage
    StdNetProtocol::parse(const Bytes& data) const
    {
        ReadOnlyMemoryStream stream(data);

        uint8_t raw_type = 0;
        if (!stream.read_exact(&raw_type, sizeof(raw_type)))
        {
            return protocol_violation_message();
        }

        const auto type = static_cast<NetMessageType>(raw_type);

        switch (type)
        {
        case NetMessageType::PING:
            return PingNetMessage();

        case NetMessageType::PONG:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
                return protocol_violation_message();

            uint8_t raw_err = 0;
            if (!stream.read_exact(&raw_err, sizeof(raw_err)))
                return protocol_violation_message();

            Bytes payload;
            if (!stream.read_bytes(payload, true))
                return protocol_violation_message();

            return PongNetMessage(session_id, static_cast<NetErrorCode>(raw_err), payload);
        }

        case NetMessageType::QUERY:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            std::string query;
            if (!stream.read_string(query, true))
            {
                return protocol_violation_message();
            }

            return QueryNetMessage(session_id, query);
        }

        case NetMessageType::CREATE_DB:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            std::string db_name;
            if (!stream.read_string(db_name, true))
            {
                return protocol_violation_message();
            }

            return CreateDbNetMessage(session_id, db_name);
        }

        case NetMessageType::ATTACH_DB:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            std::string db_name;
            if (!stream.read_string(db_name, true))
            {
                return protocol_violation_message();
            }

            return AttachDbNetMessage(session_id, db_name);
        }

        case NetMessageType::CLOSE:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            return CloseNetMessage(session_id);
        }

        case NetMessageType::UNDEFINED:
            return protocol_violation_message();
        }

        return protocol_violation_message();
    }
} // namespace net
