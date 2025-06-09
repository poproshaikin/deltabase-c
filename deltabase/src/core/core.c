#include "include/core.h"
#include "include/path_service.h"

int create_database(const char *name) {
    char path[256];
    path_db(name, path, 256);

    return 0;
}
