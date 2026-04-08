//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_NET_MESSAGE_HPP
#define DELTABASE_NET_MESSAGE_HPP

#include "UUID.hpp"

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
        SYSTEM,
        CLOSE
    };

    namespace detail
    {
        template <typename T>
        concept NetMessage_c = requires(const T& x) {
            T::type;
            x.session_id;

            requires std::same_as<decltype(T::type), const NetMessageType>;
            requires std::same_as<decltype(x.session_id), const UUID>;
        };

        template <NetMessage_c... T> using NetMessageVariant = std::variant<T...>;
    } // namespace detail

    struct PingNetMessage
    {
        static constexpr auto type = NetMessageType::PING;
        const UUID session_id = UUID::null();
    };

    struct PongNetMessage
    {
        const UUID session_id;
        static constexpr auto type = NetMessageType::PONG;

        PongNetMessage(const UUID& session_id) : session_id(session_id)
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
        static constexpr auto type = NetMessageType::QUERY;
        const UUID session_id;

        std::string db_name;

        CreateDbNetMessage(const UUID& session_id, const std::string& db_name)
            : session_id(session_id), db_name(db_name)
        {
        }
    };

    struct AttachDbNetMessage
    {
        static constexpr auto type = NetMessageType::QUERY;
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
