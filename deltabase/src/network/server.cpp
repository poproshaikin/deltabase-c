//
// Created by poproshaikin on 4/7/26.
//

#include "include/server.hpp"

#include "logger.hpp"
#include "protocol_factory.hpp"
#include "include/query_result_serializer.hpp"
#include "utils.hpp"

#include <thread>

namespace net
{
    using namespace types;
    using namespace misc;

    void
    NetServer::handle_message(SocketHandle& handle, bool& stop)
    {
        auto message_bytes = handle.receive_message();
        if (!message_bytes)
        {
            Logger::warn("Disconnecting client: " + get_ip(handle.addr()));
            handle.close();
            stop = true;
            return;
        }

        auto message = protocol_->parse(message_bytes.value());

        auto stop_with_error = [&](UUID session_id, NetErrorCode err)
        {
            Logger::info(
                "Disconnecting session with error " + std::to_string(static_cast<int>(err)) + " : "
                + session_id.
                to_string());
            auto pong_bytes = protocol_->encode(PongNetMessage(session_id, err));
            handle.send_message(pong_bytes);
            handle.close();
            stop = true;
        };

        auto stop_with_protocol_violation = [&](UUID session_id)
        {
            stop_with_error(session_id, NetErrorCode::PROTOCOL_VIOLATION);
        };

        auto get_session = [&](UUID session_id)-> engine::Engine*
        {
            std::lock_guard lock(sessions_mutex_);
            if (!sessions_.contains(session_id))
            {
                return nullptr;
            }
            return &sessions_.at(session_id);
        };

        auto send_success = [&](UUID session_id)
        {
            auto pong_bytes = protocol_->encode(PongNetMessage(session_id, NetErrorCode::SUCCESS));
            handle.send_message(pong_bytes);
        };

        QueryResultSerializer result_serializer;

        if (std::holds_alternative<PingNetMessage>(message))
        {
            stop_with_protocol_violation(UUID::null());
            return;
        }

        if (std::holds_alternative<PongNetMessage>(message))
        {
            stop_with_protocol_violation(UUID::null());
            return;
        }

        if (std::holds_alternative<QueryNetMessage>(message))
        {
            const auto& query_message = std::get<QueryNetMessage>(message);
            auto engine = get_session(query_message.session_id);
            if (!engine)
            {
                stop_with_protocol_violation(query_message.session_id);
                return;
            }

            try
            {
                auto result = engine->execute_query(query_message.query);
                auto result_bytes = result_serializer.serialize(*result);
                auto pong = PongNetMessage(query_message.session_id, NetErrorCode::SUCCESS, result_bytes);
                auto pong_bytes = protocol_->encode(pong);
                handle.send_message(pong_bytes);
            }
            catch (const std::exception& ex)
            {
                Logger::error(std::string("QUERY failed: ") + ex.what());
                stop_with_protocol_violation(query_message.session_id);
                return;
            }

            Logger::info(
                "Received QUERY message on session " + query_message.session_id.to_string());
            return;
        }

        if (std::holds_alternative<CreateDbNetMessage>(message))
        {
            const auto& create_db_message = std::get<CreateDbNetMessage>(message);
            auto engine = get_session(create_db_message.session_id);
            if (!engine)
            {
                stop_with_protocol_violation(create_db_message.session_id);
                return;
            }

            try
            {
                engine->create_db(Config::std(create_db_message.db_name));
                send_success(create_db_message.session_id);
            }
            catch (const std::exception& ex)
            {
                Logger::error(std::string("CREATE_DB failed: ") + ex.what());
                stop_with_protocol_violation(create_db_message.session_id);
                return;
            }

            Logger::info("Received CREATE_DB message");
            return;
        }

        if (std::holds_alternative<AttachDbNetMessage>(message))
        {
            const auto& attach_db_message = std::get<AttachDbNetMessage>(message);
            Logger::info(
                "Received ATTACH_DB message on session " + attach_db_message.session_id.
                to_string());

            auto engine = get_session(attach_db_message.session_id);
            if (!engine)
            {
                stop_with_protocol_violation(attach_db_message.session_id);
                return;
            }

            try
            {
                engine->attach_db(attach_db_message.db_name);
                send_success(attach_db_message.session_id);
            }
            catch (DbDoesntExists)
            {
                stop_with_error(attach_db_message.session_id, NetErrorCode::DB_NOT_EXISTS);
                return;
            }
            catch (const std::exception& ex)
            {
                Logger::error(std::string("ATTACH_DB failed: ") + ex.what());
                stop_with_protocol_violation(attach_db_message.session_id);
                return;
            }

            return;
        }

        if (std::holds_alternative<CloseNetMessage>(message))
        {
            const auto& close_message = std::get<CloseNetMessage>(message);
            if (!get_session(close_message.session_id))
            {
                stop_with_protocol_violation(close_message.session_id);
                return;
            }

            {
                std::lock_guard lock(sessions_mutex_);
                sessions_.erase(close_message.session_id);
            }
            handle.close();
            stop = true;
            Logger::info("Closed session " + close_message.session_id.to_string());
            return;
        }

        stop_with_protocol_violation(UUID::null());
    }

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
                [this, client = std::move(client)]() mutable
                {
                    handle_client(std::move(client));
                }
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
                UUID::null(),
                NetErrorCode::PROTOCOL_VIOLATION
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

        Logger::info("Created session " + session_id.to_string());

        bool stop = false;
        while (running_ && !stop)
        {
            handle_message(handle, stop);
        }

        {
            std::lock_guard lock(sessions_mutex_);
            sessions_.erase(session_id);
        }

        handle.close();
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