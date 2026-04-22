//
// Created by poproshaikin on 4/7/26.
//

#include "include/server.hpp"

#include "include/query_result_serializer.hpp"
#include "logger.hpp"
#include "protocol_factory.hpp"
#include "utils.hpp"

#include <thread>

namespace net
{
    using namespace types;
    using namespace misc;

    engine::Engine*
    NetServer::get_session(const UUID& session_id)
    {
        std::lock_guard lock(sessions_mutex_);
        if (!sessions_.contains(session_id))
        {
            return nullptr;
        }

        return &sessions_.at(session_id);
    }

    void
    NetServer::send_pong_and_stop(
        SocketHandle& handle,
        bool& stop,
        const UUID& session_id,
        NetErrorCode err,
        int32_t request_id,
        const std::string& error)
    {
        Logger::info(
            "Disconnecting session with error " + std::to_string(static_cast<int>(err)) + " : "
            + session_id.to_string());

        auto pong = PongNetMessage(session_id, err, request_id);
        if (!error.empty() && err != NetErrorCode::SUCCESS)
        {
            pong.payload = Bytes(error.begin(), error.end());
        }

        auto pong_bytes = protocol_->encode(pong);
        handle.send_message(pong_bytes);
        handle.close();
        stop = true;
    }

    void
    NetServer::send_success(
        SocketHandle& handle,
        const UUID& session_id,
        int32_t request_id)
    {
        auto pong_bytes = protocol_->encode(
            PongNetMessage(session_id, NetErrorCode::SUCCESS, request_id));
        handle.send_message(pong_bytes);
    }

    void
    NetServer::handle_query_message(
        SocketHandle& handle,
        bool& stop,
        const QueryNetMessage& message)
    {
        auto engine = get_session(message.session_id);
        if (!engine)
        {
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::UNINITIALIZED_SESSION,
                message.request_id);
            return;
        }

        try
        {
            std::unique_ptr<IExecutionResult> result = engine->execute_query(message.query);
            handle_stream(
                handle,
                message.session_id,
                message.request_id,
                std::move(result),
                stop);
        }
        catch (const std::exception& ex)
        {
            Logger::error(std::string("QUERY failed: ") + ex.what());
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::SQL_ERROR,
                message.request_id,
                ex.what());
            return;
        }

        Logger::info("Received QUERY message on session " + message.session_id.to_string());
    }

    void
    NetServer::handle_create_db_message(
        SocketHandle& handle,
        bool& stop,
        const CreateDbNetMessage& message)
    {
        auto engine = get_session(message.session_id);
        if (!engine)
        {
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::UNINITIALIZED_SESSION,
                message.request_id);
            return;
        }

        try
        {
            engine->create_db(Config::std(message.db_name));
            send_success(handle, message.session_id, message.request_id);
        }
        catch (const std::exception& ex)
        {
            Logger::error(std::string("CREATE_DB failed: ") + ex.what());
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::SQL_ERROR,
                message.request_id,
                ex.what());
            return;
        }

        Logger::info("Received CREATE_DB message");
    }

    void
    NetServer::handle_attach_db_message(
        SocketHandle& handle,
        bool& stop,
        const AttachDbNetMessage& message)
    {
        Logger::info(
            "Received ATTACH_DB message on session " + message.session_id.to_string());

        auto engine = get_session(message.session_id);
        if (!engine)
        {
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::UNINITIALIZED_SESSION,
                message.request_id);
            return;
        }

        try
        {
            engine->attach_db(message.db_name);
            send_success(handle, message.session_id, message.request_id);
        }
        catch (DbDoesntExists)
        {
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::DB_NOT_EXISTS,
                message.request_id);
            return;
        }
        catch (const std::exception& ex)
        {
            Logger::error(std::string("ATTACH_DB failed: ") + ex.what());
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::SQL_ERROR,
                message.request_id,
                ex.what());
            return;
        }
    }

    void
    NetServer::handle_close_message(
        SocketHandle& handle,
        bool& stop,
        const CloseNetMessage& message)
    {
        if (!get_session(message.session_id))
        {
            send_pong_and_stop(
                handle,
                stop,
                message.session_id,
                NetErrorCode::UNINITIALIZED_SESSION,
                message.request_id);
            return;
        }

        {
            std::lock_guard lock(sessions_mutex_);
            sessions_.erase(message.session_id);
        }

        handle.close();
        stop = true;
        Logger::info("Closed session " + message.session_id.to_string());
    }

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
        const auto request_id = std::visit([](const auto& msg)
        {
            return msg.request_id;
        }, message);

        if (std::holds_alternative<PingNetMessage>(message))
        {
            send_pong_and_stop(
                handle,
                stop,
                UUID::null(),
                NetErrorCode::PROTOCOL_VIOLATION,
                request_id);
            return;
        }

        if (std::holds_alternative<PongNetMessage>(message))
        {
            send_pong_and_stop(
                handle,
                stop,
                UUID::null(),
                NetErrorCode::PROTOCOL_VIOLATION,
                request_id);
            return;
        }

        if (std::holds_alternative<QueryNetMessage>(message))
        {
            handle_query_message(handle, stop, std::get<QueryNetMessage>(message));
            return;
        }

        if (std::holds_alternative<CreateDbNetMessage>(message))
        {
            handle_create_db_message(handle, stop, std::get<CreateDbNetMessage>(message));
            return;
        }

        if (std::holds_alternative<AttachDbNetMessage>(message))
        {
            handle_attach_db_message(handle, stop, std::get<AttachDbNetMessage>(message));
            return;
        }

        if (std::holds_alternative<CloseNetMessage>(message))
        {
            handle_close_message(handle, stop, std::get<CloseNetMessage>(message));
            return;
        }

        send_pong_and_stop(
            handle,
            stop,
            UUID::null(),
            NetErrorCode::PROTOCOL_VIOLATION,
            request_id);
    }

    bool
    NetServer::next_chunk(IExecutionResult& result, DataTable& out, bool& has_more)
    {
        constexpr uint64_t batch_size = 1024;

        out.rows.clear();

        DataRow row;
        has_more = false;

        while (out.rows.size() < batch_size)
        {
            if (!result.next(row))
            {
                has_more = false;
                break;
            }

            out.rows.push_back(row);
            has_more = true;
        }

        return !out.rows.empty();
    }

    void
    NetServer::handle_stream(
        SocketHandle& handle,
        const UUID& session_id,
        int32_t request_id,
        std::unique_ptr<IExecutionResult>&& result,
        bool& stop)
    {
        std::atomic cancelled(false);
        std::atomic disconnected(false);

        QueryResultSerializer result_serializer;

        std::mutex send_mutex;

        auto send_chunk = [&](const DataTable& chunk)
        {
            std::lock_guard lock(send_mutex);

            auto serialized_chunk = result_serializer.serialize(chunk);
            PongNetMessage chunk_message(
                session_id,
                NetErrorCode::STREAM_CHUNK,
                request_id,
                serialized_chunk);

            auto serialized_msg = protocol_->encode(chunk_message);
            handle.send_message(serialized_msg);
        };

        auto cancel_handler = [&]
        {
            auto msg_bytes = handle.receive_message();

            if (!msg_bytes)
            {
                Logger::warn("Client disconnected: " + get_ip(handle.addr()));
                disconnected.store(true);
                cancelled.store(true);
                stop = true;
                return;
            }

            auto message = protocol_->parse(msg_bytes.value());

            std::visit(
                [&]<typename T>(const T& msg)
                {
                    if constexpr (std::is_same_v<T, CancelStreamMessage>)
                    {
                        if (msg.session_id != session_id)
                        {
                            return;
                        }

                        cancelled.store(true);

                        PongNetMessage stream_cancelled(
                            session_id,
                            NetErrorCode::SUCCESS,
                            msg.request_id);
                        auto stream_cancelled_bytes = protocol_->encode(stream_cancelled);
                        handle.send_message(stream_cancelled_bytes);
                    }
                },
                message);
        };

        auto stream_handler = [&]
        {
            PongNetMessage start_stream(session_id, NetErrorCode::START_STREAM, request_id);
            handle.send_message(protocol_->encode(start_stream));

            DataTable chunk;
            chunk.output_schema = result->output_schema();

            bool has_more = false;

            while (!cancelled.load() && next_chunk(*result, chunk, has_more))
            {
                send_chunk(chunk);
            }

            if (!cancelled.load() && !disconnected.load())
            {
                PongNetMessage end_stream(session_id, NetErrorCode::END_STREAM, request_id);
                handle.send_message(protocol_->encode(end_stream));
            }
        };

        std::thread cancel_thread(cancel_handler);
        std::thread stream_thread(stream_handler);

        stream_thread.join();
        cancelled.store(true);
        cancel_thread.join();
    }

    NetServer::NetServer(
        uint16_t port,
        Config::NetProtocolType protocol,
        Config::DomainType domain,
        Config::TransportType type)
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
                })
                .detach();
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
                NetErrorCode::PROTOCOL_VIOLATION,
                0);
            auto pong_bytes = protocol_->encode(pong);
            handle.send_message(pong_bytes);
            handle.close();
            return;
        }

        const auto ping = std::get<PingNetMessage>(msg);

        auto session_id = UUID::make();
        {
            std::lock_guard lock(sessions_mutex_);
            sessions_[session_id] = engine::Engine();
        }

        PongNetMessage pong(session_id, NetErrorCode::SUCCESS, ping.request_id);
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
