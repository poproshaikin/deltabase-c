//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_SOCKET_HANDLE_HPP
#define DELTABASE_SOCKET_HANDLE_HPP
#include "socket_api.hpp"
#include "../../types/include/config.hpp"

#include <bits/socket.h>
#include <string>
#if WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace net
{
    int
    to_af(types::Config::DomainType d);
    int
    to_socktype(types::Config::TransportType T);

    class SocketHandle
    {
        socket_t fd_ = INVALID_SOCKET;
        sockaddr_storage addr_;

    public:
        // Takes control over given FD
        explicit
        SocketHandle(int&& fd, const sockaddr_storage& sockaddr);
        ~SocketHandle();

        // Restrict copying
        SocketHandle(const SocketHandle&) = delete;
        SocketHandle&
        operator=(const SocketHandle&) = delete;
        // ----------------

        // Allow moving
        SocketHandle(SocketHandle&& other) noexcept;
        SocketHandle&
        operator=(SocketHandle&& other) noexcept;
        // ------------

        static SocketHandle
        make_listener(uint16_t port, types::Config::DomainType domain, types::Config::TransportType type, int backlog = 128);

        static SocketHandle
        make_client(
            const std::string& address,
            uint16_t port,
            types::Config::DomainType domain,
            types::Config::TransportType type
        );

        std::optional<types::Bytes>
        receive_message() noexcept;

        void
        send_message(const types::Bytes& content);

        SocketHandle
        accept();

        socket_t
        fd() const;

        sockaddr_storage
        addr() const;

        void
        close();

        int
        release();
    };
} // namespace net

#endif // DELTABASE_SOCKET_HANDLE_HPP
