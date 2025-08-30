#include "../src/engine/include/engine.hpp"

void test_insert() {
    std::string table_name = "eretik"; 
    std::string db_name = "helo";

    engine::DltEngine engine(db_name);

    for (int i = 0; i < 100; i++) {
        engine.run_query("INSERT INTO " + table_name + " (id, name, age) VALUES (151, 'some fucking not much long string', 18593.5134)");
    }
}

int main() {
    test_insert();
}