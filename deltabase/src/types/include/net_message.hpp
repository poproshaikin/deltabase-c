//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_NET_MESSAGE_HPP
#define DELTABASE_NET_MESSAGE_HPP

#include "UUID.hpp"
#include "net_error.hpp"
#include "typedefs.hpp"

#include <concepts>
#include <variant>

namespace types
{
    enum class NetMessageType : uint8_t
    {
        UNDEFINED,
        PING,
        PONG,
        QUERY,
        CREATE_DB,
        ATTACH_DB,
        CLOSE,
        CANCEL_STREAM
    };

    namespace detail
    {
        template <typename T>
        concept NetMessage_c = requires(const T& x)
        {
            T::type;

            requires std::same_as<decltype(T::type), const NetMessageType>;
        };

        template <NetMessage_c... T> using NetMessageVariant = std::variant<T...>;
    } // namespace detail

    struct PingNetMessage
    {
        static constexpr auto type = NetMessageType::PING;

        int32_t request_id;

        explicit PingNetMessage(const int32_t request_id) : request_id(request_id)
        {
        }
    };

    struct PongNetMessage
    {
        static constexpr auto type = NetMessageType::PONG;
        const UUID session_id;
        NetErrorCode err;
        int32_t request_id;

        Bytes payload;

        PongNetMessage(const UUID& session_id, NetErrorCode err, int32_t request_id, const Bytes& payload = {}) :
            session_id(session_id), err(err), request_id(request_id), payload(payload)
        {
        }
    };

    struct QueryNetMessage
    {
        static constexpr auto type = NetMessageType::QUERY;
        const UUID session_id;
        int32_t request_id;

        std::string query;

        QueryNetMessage(const UUID& session_id, int32_t request_id, const std::string& query)
            : session_id(session_id), request_id(request_id), query(query)
        {
        }
    };

    struct CreateDbNetMessage
    {
        static constexpr auto type = NetMessageType::CREATE_DB;
        const UUID session_id;
        int32_t request_id;

        std::string db_name;

        CreateDbNetMessage(const UUID& session_id, int32_t request_id, const std::string& db_name)
            : session_id(session_id), request_id(request_id), db_name(db_name)
        {
        }
    };

    struct AttachDbNetMessage
    {
        static constexpr auto type = NetMessageType::ATTACH_DB;
        const UUID session_id;
        int32_t request_id;

        std::string db_name;

        AttachDbNetMessage(const UUID& session_id, int32_t request_id, const std::string& db_name)
            : session_id(session_id), request_id(request_id), db_name(db_name)
        {
        }
    };

    struct CancelStreamMessage
    {
        static constexpr auto type = NetMessageType::CANCEL_STREAM;
        const UUID session_id;
        int32_t request_id;

        CancelStreamMessage(const UUID& session_id, int32_t request_id)
            : session_id(session_id), request_id(request_id)
        {
        }
    };

    struct CloseNetMessage
    {
        static constexpr auto type = NetMessageType::CLOSE;
        const UUID session_id;
        int32_t request_id;

        CloseNetMessage(const UUID& session_id, int32_t request_id)
            : session_id(session_id), request_id(request_id)
        {
        }
    };

    using NetMessage = detail::NetMessageVariant<
        PingNetMessage,
        PongNetMessage,
        QueryNetMessage,
        CreateDbNetMessage,
        AttachDbNetMessage,
        CancelStreamMessage,
        CloseNetMessage>;
} // namespace types

#endif // DELTABASE_NET_MESSAGE_HPP