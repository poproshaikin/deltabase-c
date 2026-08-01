#include "../cli/include/cli.hpp"
#include "static_storage.hpp"

#include <iostream>

int
main(int argc, char** argv)
{
    misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());

    std::string attached_db;
    for (int i = 1; i < argc - 1; ++i)
        if (std::string(argv[i]) == "--db")
            attached_db = argv[i + 1];

    cli::CliContext ctx{
        .running = true,
        .attached_db = attached_db,
        .in = std::cin,
        .out = std::cout
    };

    cli::Cli cli(ctx);
    cli.run();
}