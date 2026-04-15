#include "../src/engine/include/engine.hpp"
#include "../src/network/include/socket_handle.hpp"
#include "../src/network/include/std_protocol.hpp"
#include "static_storage.hpp"

int main(int argc, char** argv) {
    auto handle = net::SocketHandle::make_client(
        "127.0.0.1",
        8989,
        types::Config::DomainType::IPv4,
        types::Config::TransportType::Stream
    );
    net::StdNetProtocol protocol;
    auto msg = types::PingNetMessage{};
    auto ping_bytes = protocol.encode(types::NetMessage(msg));

    handle.send_message(ping_bytes);
    auto pong_bytes = handle.receive_message();
    if (!pong_bytes)
        throw std::runtime_error("debil");

    auto pong = std::get<types::PongNetMessage>(protocol.parse(pong_bytes.value()));

    std::cout << static_cast<int>(pong.type) << std::endl;
    std::cout << pong.session_id.to_string() << std::endl;
    std::cout << static_cast<int>(pong.err) << std::endl;

    while (true)
    {

    }
}