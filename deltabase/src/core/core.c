#include "include/core.h"
#include "include/path_service.h"
#include "include/utils.h"

#include <sys/stat.h>

int create_database(const char *name) {
    char buffer[256];
    path_db(name, buffer, 256);

    if (dir_exists(buffer)) {
        return 1;
    }

    int created_base = mkdir(buffer, 0755);
    if (created_base != 0) {
        return 2;
    }

    path_db_meta(name, buffer, 256);
    int created_meta = mkdir(buffer, 0755);
    if (created_meta != 0) {
        return 3;
    }

    path_db_meta_tables(name, buffer, 256);
    int created_table = create_file(buffer);
    if (created_table != 0) {
        return 4;
    }

    path_db_meta_columns(name, buffer, 256);
    int created_column = create_file(buffer);
    if (created_column != 0) {
        return 5;
    }
       
    return 0;
}

int drop_database(const char *name) {
    char db_path[256];
    path_db(name, db_path, sizeof(db_path));

    if (!dir_exists(db_path)) {
        fprintf(stderr, "Database '%s' doesn't exists\n", name);
        return 1;
    }

    int result = rmdir_recursive(db_path);

    if (result == 0) {
        printf("Database '%s' dropped successfully.\n", name);
        return 0;
    } else {
        fprintf(stderr, "Failed to drop database '%s'.\n", name);
        return 2; 
    }
}

int create_table(const char *db_name, const char *table_name, const DataSchema *schema) {
    // create a directory
    // create a .meta file
    // create a data/ directory
    
    char buffer[256];
    path_db_table(db_name, table_name, buffer, 256);
    if (dir_exists(buffer)) {
        fprintf(stderr, "Failed to create a already existing table %s\n", table_name);
        return 1;
    }

    if (mkdir(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", table_name);
        return 2;
    }   

    path_db_table_meta(db_name, table_name, buffer, 256);
    int created_meta_file = create_file(buffer);

    path_db_table_data(db_name, table_name, buffer, 256);
    if (mkdir(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", table_name);
        return 3;
    }
}
