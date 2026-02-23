#include "static_storage.hpp"
#include "../src/engine/include/engine.hpp"

int main(int argc, char** argv) {
    misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());

    auto cfg = types::Config::std("some_db");
    engine::Engine engine;
    engine.create_db(cfg);
}