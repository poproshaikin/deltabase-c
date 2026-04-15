#include "../src/engine/include/engine.hpp"
#include "static_storage.hpp"

#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
    std::string
    make_test_db_name()
    {
        const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        return std::string("concurrency_test_") + std::to_string(getpid()) + "_" + std::to_string(now);
    }

    int
    run_insert_worker(const std::string& db_name, int start_id, int rows, int start_read_fd)
    {
        char start_signal = 0;
        if (read(start_read_fd, &start_signal, 1) != 1)
        {
            return 2;
        }

        engine::Engine engine;
        engine.attach_db(db_name);

        for (int i = 0; i < rows; ++i)
        {
            const int id = start_id + i;
            const std::string query =
                std::string("insert into common.test_concurrent(id, payload) values (")
                + std::to_string(id)
                + ", 'worker_"
                + std::to_string(start_id)
                + "')";

            engine.execute_query(query);
        }

        return 0;
    }

    void
    run_concurrent_two_processes_test()
    {
        const auto executable_path = misc::StaticStorage::get_executable_path();
        const std::string db_name = make_test_db_name();
        const auto db_path = executable_path / "data" / db_name;

        std::error_code ec;
        std::filesystem::remove_all(db_path, ec);

        {
            engine::Engine bootstrap;
            bootstrap.create_db(types::Config::std(db_name));
            bootstrap.execute_query("create table common.test_concurrent(id integer, payload string)");
        }

        int start_pipe_1[2] = {-1, -1};
        int start_pipe_2[2] = {-1, -1};
        if (pipe(start_pipe_1) != 0 || pipe(start_pipe_2) != 0)
        {
            throw std::runtime_error("Failed to create synchronization pipes");
        }

        constexpr int rows_per_worker = 100;

        const pid_t worker_1 = fork();
        if (worker_1 < 0)
        {
            throw std::runtime_error("Failed to fork first worker");
        }

        if (worker_1 == 0)
        {
            close(start_pipe_1[1]);
            close(start_pipe_2[0]);
            close(start_pipe_2[1]);
            const int code = run_insert_worker(db_name, 0, rows_per_worker, start_pipe_1[0]);
            close(start_pipe_1[0]);
            _exit(code);
        }

        const pid_t worker_2 = fork();
        if (worker_2 < 0)
        {
            kill(worker_1, SIGTERM);
            throw std::runtime_error("Failed to fork second worker");
        }

        if (worker_2 == 0)
        {
            close(start_pipe_2[1]);
            close(start_pipe_1[0]);
            close(start_pipe_1[1]);
            const int code = run_insert_worker(db_name, 100000, rows_per_worker, start_pipe_2[0]);
            close(start_pipe_2[0]);
            _exit(code);
        }

        close(start_pipe_1[0]);
        close(start_pipe_2[0]);

        const char start_signal = 'S';
        if (write(start_pipe_1[1], &start_signal, 1) != 1 || write(start_pipe_2[1], &start_signal, 1) != 1)
        {
            kill(worker_1, SIGTERM);
            kill(worker_2, SIGTERM);
            throw std::runtime_error("Failed to send start signal to workers");
        }

        close(start_pipe_1[1]);
        close(start_pipe_2[1]);

        int status_1 = 0;
        int status_2 = 0;

        if (waitpid(worker_1, &status_1, 0) < 0 || waitpid(worker_2, &status_2, 0) < 0)
        {
            throw std::runtime_error("Failed waiting for workers");
        }

        const bool worker_1_ok = WIFEXITED(status_1) && WEXITSTATUS(status_1) == 0;
        const bool worker_2_ok = WIFEXITED(status_2) && WEXITSTATUS(status_2) == 0;

        if (!worker_1_ok || !worker_2_ok)
        {
            throw std::runtime_error("At least one worker failed during concurrent insert");
        }

        engine::Engine verifier;
        verifier.attach_db(db_name);

        auto result = verifier.execute_query("select * from common.test_concurrent");
        types::DataRow row;
        int total_rows = 0;
        while (result->next(row))
        {
            total_rows++;
        }

        const int expected_rows = rows_per_worker * 2;
        if (total_rows != expected_rows)
        {
            throw std::runtime_error(
                std::string("Unexpected row count after concurrent writes. Expected ")
                + std::to_string(expected_rows)
                + ", got "
                + std::to_string(total_rows)
            );
        }

        std::cout << "Concurrent multi-process insert test passed. Rows: " << total_rows << std::endl;
    }
}

int main(int argc, char** argv) {
    misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());
    // auto handle = net::SocketHandle::make_client(
    //     "127.0.0.1",
    //     8989,
    //     types::Config::DomainType::IPv4,
    //     types::Config::TransportType::Stream
    // );
    // net::StdNetProtocol protocol;
    // auto msg = types::PingNetMessage{};
    // auto ping_bytes = protocol.encode(types::NetMessage(msg));
    //
    // handle.send_message(ping_bytes);
    // auto pong_bytes = handle.receive_message();
    // if (!pong_bytes)
    //     throw std::runtime_error("debil");
    //
    // auto pong = std::get<types::PongNetMessage>(protocol.parse(pong_bytes.value()));
    //
    // std::cout << static_cast<int>(pong.type) << std::endl;
    // std::cout << pong.session_id.to_string() << std::endl;
    // std::cout << static_cast<int>(pong.err) << std::endl;
    //
    // while (true)
    // {
    //
    // }

    try
    {
        run_concurrent_two_processes_test();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Test failed: " << ex.what() << std::endl;
        return 1;
    }
}