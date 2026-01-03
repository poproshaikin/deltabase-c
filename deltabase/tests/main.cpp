#include "static_storage.hpp"
#include "../src/engine/include/engine.hpp"

int main(int argc, char** argv) {
    misc::StaticStorage::executable_path = std::filesystem::absolute(argv[0]).parent_path();

    types::Config cfg("some_db", misc::StaticStorage::executable_path);
    engine::Engine engine;
    engine.create_db(cfg);
}