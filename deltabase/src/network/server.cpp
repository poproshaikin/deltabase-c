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

        auto stop_with_error = [&](
            const UUID& session_id,
            NetErrorCode err,
            const std::string& error = "")
        {
            Logger::info(
                "Disconnecting session with error " + std::to_string(static_cast<int>(err)) + " : "
                + session_id.
                to_string());

            auto pong = PongNetMessage(session_id, err);
            if (!error.empty() && err != NetErrorCode::SUCCESS)
                pong.payload = Bytes(error.begin(), error.end());

            auto pong_bytes = protocol_->encode(pong);
            handle.send_message(pong_bytes);
            handle.close();
            stop = true;
        };

        auto get_session = [&](const UUID& session_id)-> engine::Engine*
        {
            std::lock_guard lock(sessions_mutex_);
            if (!sessions_.contains(session_id))
            {
                return nullptr;
            }
            return &sessions_.at(session_id);
        };

        auto send_success = [&](const UUID& session_id)
        {
            auto pong_bytes = protocol_->encode(PongNetMessage(session_id, NetErrorCode::SUCCESS));
            handle.send_message(pong_bytes);
        };

        if (std::holds_alternative<PingNetMessage>(message))
        {
            stop_with_error(UUID::null(), NetErrorCode::PROTOCOL_VIOLATION);
            return;
        }

        if (std::holds_alternative<PongNetMessage>(message))
        {
            stop_with_error(UUID::null(), NetErrorCode::PROTOCOL_VIOLATION);
            return;
        }

        if (std::holds_alternative<QueryNetMessage>(message))
        {
            const auto& query_message = std::get<QueryNetMessage>(message);
            auto engine = get_session(query_message.session_id);
            if (!engine)
            {
                stop_with_error(query_message.session_id, NetErrorCode::UNINITIALIZED_SESSION);
                return;
            }

            try
            {
                std::unique_ptr<IExecutionResult> result = engine->execute_query(
                    query_message.query);

                handle_stream(handle, query_message.session_id, std::move(result), stop);
            }
            catch (const std::exception& ex)
            {
                Logger::error(std::string("QUERY failed: ") + ex.what());
                stop_with_error(query_message.session_id, NetErrorCode::SQL_ERROR, ex.what());
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
                stop_with_error(create_db_message.session_id, NetErrorCode::UNINITIALIZED_SESSION);
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
                stop_with_error(create_db_message.session_id, NetErrorCode::SQL_ERROR, ex.what());
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
                stop_with_error(attach_db_message.session_id, NetErrorCode::UNINITIALIZED_SESSION);
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
                stop_with_error(attach_db_message.session_id, NetErrorCode::SQL_ERROR, ex.what());
                return;
            }

            return;
        }

        if (std::holds_alternative<CloseNetMessage>(message))
        {
            const auto& close_message = std::get<CloseNetMessage>(message);
            if (!get_session(close_message.session_id))
            {
                stop_with_error(close_message.session_id, NetErrorCode::UNINITIALIZED_SESSION);
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

        stop_with_error(UUID::null(), NetErrorCode::PROTOCOL_VIOLATION);
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
                serialized_chunk
            );

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

            std::visit([&]<typename T>(const T& msg)
                       {
                           if constexpr (std::is_same_v<T, CancelStreamMessage>)
                           {
                               if (msg.session_id != session_id)
                                   return;

                               cancelled.store(true);

                               PongNetMessage stream_cancelled(session_id, NetErrorCode::SUCCESS);
                               auto stream_cancelled_bytes = protocol_->encode(stream_cancelled);
                               handle.send_message(stream_cancelled_bytes);
                           }

                       },
                       message);
        };

        auto stream_handler = [&]
        {
            PongNetMessage start_stream(session_id, NetErrorCode::START_STREAM);
            handle.send_message(protocol_->encode(start_stream));

            DataTable chunk;
            chunk.output_schema = result->output_schema();

            bool has_more = false;

            while (!cancelled.load() &&
                   next_chunk(*result, chunk, has_more))
            {
                send_chunk(chunk);
            }

            if (!cancelled.load() && !disconnected.load())
            {
                PongNetMessage end_stream(session_id, NetErrorCode::END_STREAM);
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