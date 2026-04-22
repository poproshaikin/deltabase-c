//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_SERVER_HPP
#define DELTABASE_SERVER_HPP
#include "engine.hpp"
#include "protocol.hpp"
#include "session.hpp"
#include "socket_handle.hpp"

#include <cstdint>

#include <atomic>

namespace net
{
    class NetServer
    {
        std::atomic_bool running_{false};
        SocketHandle listener_;
        uint16_t port_;

        std::unordered_map<types::UUID, engine::Engine> sessions_;
        std::mutex sessions_mutex_;

        std::unique_ptr<INetProtocol> protocol_;

        void
        handle_client(SocketHandle handle);

        void
        handle_message(SocketHandle& handle, bool& stop);

        engine::Engine*
        get_session(const types::UUID& session_id);

        void
        send_pong_and_stop(
            SocketHandle& handle,
            bool& stop,
            const types::UUID& session_id,
            types::NetErrorCode err,
            int32_t request_id,
            const std::string& error = "");

        void
        send_success(
            SocketHandle& handle,
            const types::UUID& session_id,
            int32_t request_id);

        void
        handle_query_message(
            SocketHandle& handle,
            bool& stop,
            const types::QueryNetMessage& message);

        void
        handle_create_db_message(
            SocketHandle& handle,
            bool& stop,
            const types::CreateDbNetMessage& message);

        void
        handle_attach_db_message(
            SocketHandle& handle,
            bool& stop,
            const types::AttachDbNetMessage& message);

        void
        handle_close_message(
            SocketHandle& handle,
            bool& stop,
            const types::CloseNetMessage& message);

        bool
        next_chunk(types::IExecutionResult& result, types::DataTable& out, bool& has_more);

        void
        handle_stream(
            SocketHandle& handle,
            const types::UUID& session_id,
            int32_t request_id,
            std::unique_ptr<types::IExecutionResult>&& result,
            bool& stop);

    public:
        NetServer(
            uint16_t port,
            types::Config::NetProtocolType protocol_type,
            types::Config::DomainType domain,
            types::Config::TransportType type
        );
        ~NetServer();

        void
        start();

        void
        stop();
    };
} // namespace net

#endif // DELTABASE_SERVER_HPP
