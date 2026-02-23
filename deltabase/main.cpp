#include "cli.hpp"
#include "static_storage.hpp"

#include <iostream>

int
main(int, char** argv)
{
    misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());

    cli::CliContext ctx{
        .running = true,
        .in = std::cin,
        .out = std::cout
    };

    cli::Cli cli(ctx);
    cli.run();
}