#include "../src/engine/include/engine.hpp"

int main() {
    types::Config cfg("some_db");
    engine::Engine engine;
    engine.create_db(cfg);
}