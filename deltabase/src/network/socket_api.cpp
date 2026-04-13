//
// Created by poproshaikin on 4/13/26.
//

#include "include/socket_api.hpp"

#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socklen_t = int;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace net
{
    ssize_t
    receive(socket_t fd, void* buf, size_t len, int flags)
    {
#ifdef _WIN32
        return ::recv(fd, static_cast<char*>(buf), static_cast<int>(len), flags);
#else
        return ::recv(fd, buf, len, flags);
#endif
    }

    ssize_t
    receive_full(socket_t fd, void* buf, size_t len, int flags)
    {
        size_t total = 0;
        char* ptr = static_cast<char*>(buf);

        while (total < len)
        {
            ssize_t n = receive(fd, ptr + total, len - total, 0);

            if (n <= 0)
                return n; // error or disconnect

            total += static_cast<size_t>(n);
        }

        return static_cast<ssize_t>(total);
    }

    ssize_t
    send_full(socket_t fd, const void* buf, size_t len, int flags)
    {
        const char* ptr = static_cast<const char*>(buf);
        size_t total = 0;

        while (total < len)
        {
#ifdef _WIN32
            int n = ::send(fd, ptr + total, static_cast<size_t>(len - total), flags);
            if (n == SOCKET_ERROR)
                return -1;
#else
            ssize_t n = ::send(fd, ptr + total, len - total, flags);
            if (n < 0)
                return n;
#endif
            if (n == 0)
                return total;

            total += static_cast<size_t>(n);
        }

        return static_cast<ssize_t>(total);
    }

    socket_t
    accept_socket(socket_t listener, sockaddr_storage* addr)
    {
        socklen_t len = sizeof(*addr);

        socket_t client = ::accept(listener, reinterpret_cast<sockaddr*>(&addr), &len);
        if (!is_valid_socket(client))
            throw std::runtime_error("Failed to accept socket");

        return client;
    }

    void
    close_socket(socket_t fd)
    {
#ifdef _WIN32
        ::closesocket(fd);
#else
        ::close(fd);
#endif
    }

    bool
    is_valid_socket(socket_t fd)
    {
#ifdef _WIN32
        if (fd == INVALID_SOCKET)
#else
        if (fd < 0)
#endif
            return false;

        return true;
    }

    socket_t
    create_listener(uint16_t port, int af, int socktype, int backlog, sockaddr_storage* sockaddr)
    {
        socket_t fd = socket(af, socktype, 0);
        if (!is_valid_socket(fd))
            throw std::runtime_error("Failed to open socket");

        int opt = 1;
#ifdef _WIN32
        if (setsockopt(
                fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt)
            ) < 0)
#else
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
#endif
        {
            close_socket(fd);
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }

        if (af == AF_INET)
        {
            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (sockaddr)
                std::memcpy(sockaddr, &addr, sizeof(addr));

            if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
            {
                close_socket(fd);
                throw std::runtime_error("Failed to bind socket");
            }
        }
        else
        {
            close_socket(fd);
            throw std::runtime_error("Domain types except IPv4 are not supported yet");
        }

        if (socktype == SOCK_STREAM)
        {
            if (listen(fd, backlog) < 0)
            {
                close_socket(fd);
                throw std::runtime_error("Failed to listen on socket");
            }
        }
        else
        {
            close_socket(fd);
            throw std::runtime_error("Socket types except STREAM are not supported yet");
        }

        return fd;
    }

} // namespace net
