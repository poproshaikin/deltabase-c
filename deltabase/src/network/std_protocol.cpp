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
            return CloseNetMessage(types::UUID::null(), 0);
        }
    } // namespace

    Bytes
    StdNetProtocol::encode(const PingNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write(&msg.request_id, sizeof(msg.request_id));
        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const PongNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write(&msg.request_id, sizeof(msg.request_id));

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
        stream.write(&msg.request_id, sizeof(msg.request_id));
        stream.write_string(msg.query, true);
        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const CreateDbNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write(&msg.request_id, sizeof(msg.request_id));
        stream.write_string(msg.db_name, true);

        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const AttachDbNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write(&msg.request_id, sizeof(msg.request_id));
        stream.write_string(msg.db_name, true);

        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const CancelStreamMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write(&msg.request_id, sizeof(msg.request_id));
        return stream.to_vector();
    }

    Bytes
    StdNetProtocol::encode(const CloseNetMessage& msg) const
    {
        MemoryStream stream;
        stream.write_message_type(msg.type);
        stream.write_uuid(msg.session_id);
        stream.write(&msg.request_id, sizeof(msg.request_id));
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
        {
            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
            {
                return protocol_violation_message();
            }

            return PingNetMessage(request_id);
        }

        case NetMessageType::PONG:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
                return protocol_violation_message();

            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
                return protocol_violation_message();

            uint8_t raw_err = 0;
            if (!stream.read_exact(&raw_err, sizeof(raw_err)))
                return protocol_violation_message();

            Bytes payload;
            if (!stream.read_bytes(payload, true))
                return protocol_violation_message();

            return PongNetMessage(session_id, static_cast<NetErrorCode>(raw_err), request_id, payload);
        }

        case NetMessageType::QUERY:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
            {
                return protocol_violation_message();
            }

            std::string query;
            if (!stream.read_string(query, true))
            {
                return protocol_violation_message();
            }

            return QueryNetMessage(session_id, request_id, query);
        }

        case NetMessageType::CREATE_DB:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
            {
                return protocol_violation_message();
            }

            std::string db_name;
            if (!stream.read_string(db_name, true))
            {
                return protocol_violation_message();
            }

            return CreateDbNetMessage(session_id, request_id, db_name);
        }

        case NetMessageType::ATTACH_DB:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
            {
                return protocol_violation_message();
            }

            std::string db_name;
            if (!stream.read_string(db_name, true))
            {
                return protocol_violation_message();
            }

            return AttachDbNetMessage(session_id, request_id, db_name);
        }

        case NetMessageType::CANCEL_STREAM:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
            {
                return protocol_violation_message();
            }

            return CancelStreamMessage(session_id, request_id);
        }

        case NetMessageType::CLOSE:
        {
            UUID session_id;
            if (!stream.read_uuid(session_id))
            {
                return protocol_violation_message();
            }

            int32_t request_id = 0;
            if (!stream.read_exact(&request_id, sizeof(request_id)))
            {
                return protocol_violation_message();
            }

            return CloseNetMessage(session_id, request_id);
        }

        case NetMessageType::UNDEFINED:
            return protocol_violation_message();
        }

        return protocol_violation_message();
    }
} // namespace net
