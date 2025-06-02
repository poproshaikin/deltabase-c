#include <cstdio>
extern "C" {
#include "../core/include/core.h"
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
