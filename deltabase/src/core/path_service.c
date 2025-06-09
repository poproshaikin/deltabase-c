#include "include/path_service.h"

#include <stdio.h>

static const char *BASE_DB_PATH = "data";

void path_db(const char *db_name, char *out_result, size_t buf_size) {
    snprintf(out_result, buf_size, "%s/%s", BASE_DB_PATH, db_name);
}
