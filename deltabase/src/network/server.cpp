//
// Created by poproshaikin on 4/7/26.
//

#include "include/server.hpp"

#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace net
{
    using namespace types;

    int
    create_listening_socket(uint16_t port)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
            throw std::runtime_error("Failed to open socket");

        int opt = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            close(fd);
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            close(fd);
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(fd, 128) < 0)
        {
            close(fd);
            throw std::runtime_error("Failed to listen on socket");
        }

        return fd;
    }
    NetServer::NetServer(uint16_t port, INetTransport& transport) : port_(port), transport_(transport)
    {
        server_fd_ = create_listening_socket(port);
    }

    NetServer::~NetServer()
    {
        close(server_fd_);
    }

    void
    NetServer::start()
    {
        running_ = true;
        std::cout << "Server started at port " << port_ << std::endl;

        while (running_)
        {
            int client_fd = accept(server_fd_, nullptr, nullptr);
            if (client_fd < 0)
            {
                perror("Accept failed");
                continue;
            }

            std::thread([this, client_fd]{ handleClient(client_fd); }).detach();
        }
    }

    void
    NetServer::handleClient(int client_fd)
    {
        UUID id = UUID::make();

        PongNetMessage pong(id);

    }

    void
    NetServer::stop()
    {
        running_ = false;
        if (server_fd_ > 0)
        {
            close(server_fd_);
            server_fd_ = -1;
        }
    }
} // namespace net
