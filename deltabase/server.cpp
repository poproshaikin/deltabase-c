//
// Created by poproshaikin on 4/13/26.
//

#include "src/network/include/server.hpp"

#include "static_storage.hpp"

int
main(int, char** argv)
{
    misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());

    net::NetServer server(
        8989,
        types::Config::NetProtocolType::Std,
        types::Config::DomainType::IPv4,
        types::Config::TransportType::Stream
    );

    server.start();
}
