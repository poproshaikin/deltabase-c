#include <cstdio>
extern "C" {
    #include "../src/core/include/core.h"
    #include "../src/core/include/data_io.h"
}

using namespace std;

int main() {
    FILE *file = fopen("hello.txt", "w+");

    PageHeader header = {
        .page_id = 1,
        .rows_count = 3,
    };

    write_ph(&header, file);
}
