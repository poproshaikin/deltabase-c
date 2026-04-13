//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_SOCKET_API_HPP
#define DELTABASE_SOCKET_API_HPP
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef _WIN32
using socket_t = SOCKET;
#else
using socket_t = int;
#endif

namespace net
{
#ifdef _WIN32
    constexpr socket_t INVALID_SOCKET = ::INVALID_SOCKET
#else
    constexpr socket_t INVALID_SOCKET = -1;
#endif

    socket_t
    create_listener(
        uint16_t port, int af, int socktype, int backlog, sockaddr_storage* sockaddr = nullptr
    );

    ssize_t
    receive(socket_t fd, void* buf, size_t len, int flags);

    ssize_t
    receive_full(socket_t fd, void* buf, size_t len, int flags);

    ssize_t
    send_full(socket_t fd, const void* buf, size_t len, int flags);

    socket_t
    accept_socket(socket_t listener, sockaddr_storage* sockaddr = nullptr);

    void
    close_socket(socket_t fd);

    bool
    is_valid_socket(socket_t fd);
} // namespace net

#endif // DELTABASE_SOCKET_API_HPP
