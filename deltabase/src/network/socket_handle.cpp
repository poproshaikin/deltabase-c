//
// Created by poproshaikin on 4/13/26.
//

#include "include/socket_handle.hpp"

#include "exceptions.hpp"
#include "include/globals.hpp"
#include "include/socket_api.hpp"
#include "typedefs.hpp"

#include <netinet/in.h>
#include <stdexcept>

namespace net
{
    using namespace types;

    int
    to_af(Config::DomainType d)
    {
        switch (d)
        {
        case Config::DomainType::IPv4:
            return AF_INET;
        case Config::DomainType::IPv6:
            return AF_INET6;
        case Config::DomainType::UNIX:
            return AF_UNIX;
        }
        return AF_INET;
    }

    int
    // ReSharper disable once CppDFAConstantFunctionResult
    to_socktype(Config::TransportType T)
    {
        switch (T)
        {
        case Config::TransportType::Stream:
            return SOCK_STREAM;
        }

        return SOCK_STREAM;
    }

    SocketHandle::SocketHandle(int&& fd, const sockaddr_storage& sockaddr)
    {
        if (!is_valid_socket(fd))
            throw std::runtime_error("Failed to init Socket Handle: invalid FD");

        fd_ = fd;
        addr_ = sockaddr;
    }

    SocketHandle::~SocketHandle()
    {
        close();
    }

    SocketHandle::SocketHandle(SocketHandle&& other) noexcept : fd_(other.fd_), addr_(other.addr_)
    {
        other.fd_ = -1;
        other.addr_ = {};
    }

    SocketHandle&
    SocketHandle::operator=(SocketHandle&& other) noexcept
    {
        if (this != &other)
        {
            close();
            fd_ = other.release();
        }
        return *this;
    }

    std::optional<Bytes>
    SocketHandle::receive_message() noexcept
    {
        uint32_t len = 0;

        ssize_t r = receive_full(fd(), &len, sizeof(len), 0);
        if (r != sizeof(len))
            return std::nullopt;

        len = ntohl(len);

        if (len > MAX_INCOMING_LEN)
            return std::nullopt;

        Bytes msg;
        msg.resize(len);

        r = receive_full(fd(), msg.data(), len, 0);
        if (r != static_cast<ssize_t>(len))
            return std::nullopt;

        return msg;
    }

    void
    SocketHandle::send_message(const Bytes& content)
    {
        uint32_t len = static_cast<uint32_t>(content.size());
        uint32_t net_len = htonl(len);

        if (send_full(fd(), &net_len, sizeof(net_len), 0) != sizeof(net_len))
            throw NetworkError("Failed to write length to socket");

        if (content.empty())
            return;

        ssize_t sent = send_full(fd(), content.data(), content.size(), 0);

        if (sent != static_cast<ssize_t>(content.size()))
            throw NetworkError("Failed to write message body");
    }

    SocketHandle
    SocketHandle::accept()
    {
        sockaddr_storage addr;
        socket_t accepted = accept_socket(fd_, &addr);
        return SocketHandle(std::move(accepted), addr);
    }

    int
    SocketHandle::fd() const
    {
        return fd_;
    }

    sockaddr_storage
    SocketHandle::addr() const
    {
        return addr_;
    }

    void
    SocketHandle::close()
    {
        if (is_valid_socket(fd_))
        {
            close_socket(fd_);
        }

        fd_ = INVALID_SOCKET;
    }

    int
    SocketHandle::release()
    {
        const auto fd = fd_;
        fd_ = INVALID_SOCKET;
        return fd;
    }

    SocketHandle
    SocketHandle::make_listener(
        uint16_t port, Config::DomainType domain, Config::TransportType type, int backlog
    )
    {
        int af = to_af(domain);
        int socktype = to_socktype(type);

        sockaddr_storage addr;
        socket_t socket = create_listener(port, af, socktype, backlog, &addr);

        SocketHandle handle(std::move(socket), addr);
        return handle;
    }

    SocketHandle
    SocketHandle::make_client(
        const std::string& address,
        uint16_t port,
        Config::DomainType domain,
        Config::TransportType type
    )
    {
        int af = to_af(domain);
        int socktype = to_socktype(type);

        sockaddr_storage addr;
        socket_t socket = create_client(address, port, af, socktype, &addr);

        SocketHandle handle(std::move(socket), addr);
        return handle;
    }
} // namespace net
