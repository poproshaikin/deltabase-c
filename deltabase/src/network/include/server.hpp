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

        bool
        next_chunk(types::IExecutionResult& result, types::DataTable& out, bool& has_more);

        void
        handle_stream(SocketHandle& handle, const types::UUID& session_id, std::unique_ptr<types::IExecutionResult>&& result, bool& stop);

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
