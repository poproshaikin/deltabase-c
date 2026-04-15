//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_NET_MESSAGE_HPP
#define DELTABASE_NET_MESSAGE_HPP

#include "UUID.hpp"
#include "net_error.hpp"

#include <concepts>
#include <variant>

namespace types
{
    enum class NetMessageType
    {
        UNDEFINED,
        PING,
        PONG,
        QUERY,
        CREATE_DB,
        ATTACH_DB,
        CLOSE
    };

    namespace detail
    {
        template <typename T>
        concept NetMessage_c = requires(const T& x) {
            T::type;

            requires std::same_as<decltype(T::type), const NetMessageType>;
        };

        template <NetMessage_c... T> using NetMessageVariant = std::variant<T...>;
    } // namespace detail

    struct PingNetMessage
    {
        static constexpr auto type = NetMessageType::PING;
    };

    struct PongNetMessage
    {
        static constexpr auto type = NetMessageType::PONG;
        const UUID session_id;
        NetErrorCode err;

        PongNetMessage(const UUID& session_id, NetErrorCode err) : session_id(session_id), err(err)
        {
        }
    };

    struct QueryNetMessage
    {
        static constexpr auto type = NetMessageType::QUERY;
        const UUID session_id;

        std::string query;

        QueryNetMessage(const UUID& session_id, const std::string& query)
            : session_id(session_id), query(query)
        {
        }
    };

    struct CreateDbNetMessage
    {
        static constexpr auto type = NetMessageType::CREATE_DB;
        const UUID session_id;

        std::string db_name;

        CreateDbNetMessage(const UUID& session_id, const std::string& db_name)
            : session_id(session_id), db_name(db_name)
        {
        }
    };

    struct AttachDbNetMessage
    {
        static constexpr auto type = NetMessageType::ATTACH_DB;
        const UUID session_id;

        std::string db_name;

        AttachDbNetMessage(const UUID& session_id, const std::string& db_name)
            : session_id(session_id), db_name(db_name)
        {
        }
    };

    struct CloseNetMessage
    {
        static constexpr auto type = NetMessageType::CLOSE;
        const UUID session_id;

        CloseNetMessage(const UUID& session_id) : session_id(session_id)
        {
        }
    };

    using NetMessage = detail::NetMessageVariant<
        PingNetMessage,
        PongNetMessage,
        QueryNetMessage,
        CreateDbNetMessage,
        AttachDbNetMessage,
        CloseNetMessage>;
} // namespace types

#endif // DELTABASE_NET_MESSAGE_HPP
