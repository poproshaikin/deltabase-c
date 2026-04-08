//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_SERVER_HPP
#define DELTABASE_SERVER_HPP
#include "engine.hpp"
#include "session.hpp"

#include "transport.hpp"
#include <cstdint>

#include <atomic>

namespace net
{
    class NetServer
    {
        int server_fd_ = -1;
        uint16_t port_;
        std::atomic_bool running_{false};
        std::unordered_map<types::UUID, engine::Engine> sessions_;
        std::mutex sessions_mutex_;
        INetTransport& transport_;

        void
        handleClient(int client_fd);

    public:
        NetServer(uint16_t port, INetTransport& transport);
        ~NetServer();

        void
        start();

        void
        stop();
    };
} // namespace net

#endif // DELTABASE_SERVER_HPP
