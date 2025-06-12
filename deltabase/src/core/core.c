#include "include/core.h"
#include "include/path_service.h"
#include "include/utils.h"
#include "include/data_io.h"

#include <linux/limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

int create_database(const char *name) {
    char buffer[PATH_MAX];
    path_db(name, buffer, PATH_MAX);

    printf("%s\n", buffer);

    if (dir_exists(buffer)) {
        return 1;
    }

    int created_base = mkdir_recursive(buffer, 0755);
    printf("%i\n", created_base);
    if (created_base != 0) {
        return 2;
    }
       
    return 0;
}

int drop_database(const char *name) {
    char db_path[PATH_MAX];
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

int create_table(const char *db_name, const MetaTable *schema) {
    int op = 0;

    char buffer[PATH_MAX];
    path_db_table(db_name, schema->name, buffer, PATH_MAX);
    if (dir_exists(buffer)) {
        fprintf(stderr, "Failed to create a already existing table %s\n", schema->name);
        return 1;
    }

    if (mkdir_recursive(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", schema->name);
        return 2;
    }   

    path_db_table_meta(db_name, schema->name, buffer, PATH_MAX);

    FILE *meta_file = fopen(buffer, "w+");
    if (!meta_file) {
        return 3;
    }

    if ((op = write_mt(schema, fileno(meta_file))) != 0) {
        printf("create_table:write_mt %i\n", op);
        fclose(meta_file);
        return 4;
    }
    fclose(meta_file);

    path_db_table_data(db_name, schema->name, buffer, PATH_MAX);
    if (mkdir_recursive(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", schema->name);
        return 5;
    }

    return 0;
}

int create_page(const char *db_name, const char *table_name, PageHeader *out_new_page) {
    uuid_generate_time(out_new_page->page_id);
    out_new_page->rows_count = 0;

    char file_path[PATH_MAX];
    path_db_table_page(db_name, table_name, out_new_page->page_id, file_path, PATH_MAX);

    FILE *file = fopen(file_path, "w+");
    if (!file) {
        fprintf(stderr, "Failed to create page file %s\n", out_new_page->page_id);
        return 1;
    }

    if (write_ph(out_new_page, fileno(file)) != 0) {
        return 2;
    }
    fclose(file);
    return 0;
}

// if success, returns count of paths. otherwise, -1
ssize_t get_pages(const char *db_name, const char *table_name, char ***out_paths) {
    if (!out_paths) {
        fprintf(stderr, "get_pages: out_paths cannot be null");
        return -1;
    }

    char dir_path[PATH_MAX];
    path_db_table_data(db_name, table_name, dir_path, PATH_MAX);

    size_t files_count = 0;
    *out_paths = get_dir_files(dir_path, &files_count);

    return files_count;
}

int read_page_header(FILE *file, PageHeader *out) {
    int fd = fileno(file);

    if (read_ph(out, fd) != 0) {
        fprintf(stderr, "Failed to read page header\n");
        return 1;
    }
    
    return 0;
}

int get_table_schema(const char *db_name, const char *table_name, MetaTable *out) {
    char buffer[PATH_MAX];
    path_db_table_meta(db_name, table_name, buffer, PATH_MAX);
    
    FILE *file = fopen(buffer, "r");
    int fd = fileno(file);

    if (read_mt(out, fd) != 0) {
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}

int save_table_schema(const char *db_name, const char *table_name, const MetaTable *meta) {
    char buffer[PATH_MAX];
    path_db_table_meta(db_name, table_name, buffer, PATH_MAX);
    
    FILE *file = fopen(buffer, "w+");
    int fd = fileno(file);

    if (write_mt(meta, fd) != 0) {
        fclose(file);
        return 1;
    }

    fclose(file);
    return 0;
}
