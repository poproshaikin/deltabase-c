//
// Created by poproshaikin on 4/7/26.
//

#include "include/server.hpp"

#include "logger.hpp"
#include "protocol_factory.hpp"
#include "utils.hpp"

#include <thread>

namespace net
{
    using namespace types;
    using namespace misc;

    NetServer::NetServer(
        uint16_t port,
        Config::NetProtocolType protocol,
        Config::DomainType domain,
        Config::TransportType type
    )
        : listener_(SocketHandle::make_listener(port, domain, type)), port_(port)
    {
        NetProtocolFactory protocol_factory;
        protocol_ = protocol_factory.make(protocol);
    }

    void
    NetServer::start()
    {
        running_ = true;
        Logger::info("Server started at port " + std::to_string(port_));

        while (running_)
        {
            auto client = listener_.accept();
            std::thread(
                [this, client = std::move(client)]() mutable { handle_client(std::move(client)); }
            ).detach();
        }
    }

    void
    NetServer::handle_client(SocketHandle handle)
    {
        Logger::info("Accepted client: " + get_ip(handle.addr()));

        auto ping_bytes = handle.receive_message();
        if (!ping_bytes)
        {
            Logger::warn("Disconnecting client: " + get_ip(handle.addr()));
            handle.close();
            return;
        }

        auto msg = protocol_->parse(ping_bytes.value());

        if (!std::holds_alternative<PingNetMessage>(msg))
        {
            PongNetMessage pong(
                UUID::null(), NetErrorCode::PROTOCOL_VIOLATION
            );
            auto pong_bytes = protocol_->encode(pong);
            handle.send_message(pong_bytes);
            handle.close();
            return;
        }

        auto session_id = UUID::make();
        {
            std::lock_guard lock(sessions_mutex_);
            sessions_[session_id] = engine::Engine();
        }

        PongNetMessage pong(session_id, NetErrorCode::SUCCESS);
        auto pong_bytes = protocol_->encode(pong);
        handle.send_message(pong_bytes);

        while (running_)
        {
            auto message_bytes = handle.receive_message();
            if (!message_bytes)
            {
                Logger::warn("Disconnecting client: " + get_ip(handle.addr()));
                handle.close();
                return;
            }

            auto message = protocol_->parse(message_bytes.value());
            if (std::holds_alternative<CloseNetMessage>(message))
            {
                auto close_message = std::get<CloseNetMessage>(message);
                {
                    std::lock_guard lock(sessions_mutex_);
                    sessions_.erase(close_message.session_id);
                }
                handle.close();
                break;
            }
        }
    }

    void
    NetServer::stop()
    {
        running_ = false;
        listener_.close();
    }

    NetServer::~NetServer()
    {
        stop();
    }
} // namespace net
