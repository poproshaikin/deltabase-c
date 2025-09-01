#include "include/paths.h"
#include "include/utils.h"
#include <linux/limits.h>
#include <stdio.h>

int
path_data(char* out_result, size_t buf_size) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;   
    }
    snprintf(out_result, buf_size, "%s/%s", exec_dir, DATA);
    return 0;
}

int
path_db(const char* db_name, char* out_result, size_t buf_size) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;   
    }
    snprintf(out_result, buf_size, "%s/%s/%s", exec_dir, DATA, db_name);
    return 0;
}

int
path_db_schema(const char* db_name, const char* schema_name, char* out_result, size_t buf_size) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;   
    }
    snprintf(out_result, buf_size, "%s/%s/%s/%s", exec_dir, DATA, db_name, schema_name);
    return 0;
}

int
path_db_schema_meta(
    const char* db_name, const char* schema_name, char* out_result, size_t buf_size
) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;   
    }
    snprintf(out_result, buf_size, "%s/%s/%s/%s/%s", exec_dir, DATA, db_name, schema_name, META);
    return 0;
}

int
path_db_meta(const char* db_name, char* out_result, size_t buf_size) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    snprintf(out_result, buf_size, "%s/%s/%s/%s", exec_dir, DATA, db_name, META);
    return 0;
}

int
path_db_meta_tables(const char* db_name, char* out_result, size_t buf_size) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    snprintf(out_result, buf_size, "%s/%s/%s/%s/%s", exec_dir, DATA, db_name, META, META_TABLES);
    return 0;
}

int
path_db_meta_columns(const char* db_name, char* out_result, size_t buf_size) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    snprintf(out_result, buf_size, "%s/%s/%s/%s/%s", exec_dir, DATA, db_name, META, META_COLUMNS);
    return 0;
}

int
path_db_schema_table(
    const char* db_name,
    const char* schema_name,
    const char* table_name,
    char* out_result,
    size_t buf_size
) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    snprintf(out_result, buf_size, "%s/%s/%s/%s/%s", exec_dir, DATA, db_name, schema_name, table_name);
    return 0;
}

int
path_db_schema_table_meta(
    const char* db_name,
    const char* schema_name,
    const char* table_name,
    char* out_result,
    size_t buf_size
) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    snprintf(
        out_result, buf_size, "%s/%s/%s/%s/%s", exec_dir, DATA, db_name, table_name, META_TABLE_FILE
    );
    return 0;
}

int
path_db_schema_table_data(
    const char* db_name,
    const char* schema_name,
    const char* table_name,
    char* out_result,
    size_t buf_size
) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    snprintf(
        out_result,
        buf_size,
        "%s/%s/%s/%s/%s/%s",
        exec_dir,
        DATA,
        db_name,
        schema_name,
        table_name,
        DATA
    );
    return 0;
}

int
path_db_schema_table_page(
    const char* db_name,
    const char* schema_name,
    const char* table_name,
    uuid_t uuid,
    char* out_result,
    size_t buf_size
) {
    char exec_dir[PATH_MAX];
    if (get_executable_dir(exec_dir, PATH_MAX) != 0) {
        fprintf(stderr, "Failed to get executable directory\n");
        return 1;
    }
    char uuid_string[37];
    uuid_unparse_lower(uuid, uuid_string);
    snprintf(
        out_result,
        buf_size,
        "%s/%s/%s/%s/%s/%s/%s",
        exec_dir,
        DATA,
        db_name,
        schema_name,
        table_name,
        DATA,
        uuid_string
    );
    return 0;
}