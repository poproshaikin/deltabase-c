#include "include/core.h"
#include "include/path_service.h"
#include "include/utils.h"
#include "include/data_io.h"

#include <linux/limits.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

int create_database(const char *name) {
    char buffer[PATH_MAX];
    path_db(name, buffer, PATH_MAX);

    if (dir_exists(buffer)) {
        return 1;
    }

    int created_base = mkdir(buffer, 0755);
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

int create_table(const char *db_name, const char *table_name, const MetaTable *schema) {
    char buffer[PATH_MAX];
    path_db_table(db_name, table_name, buffer, PATH_MAX);
    if (dir_exists(buffer)) {
        fprintf(stderr, "Failed to create a already existing table %s\n", table_name);
        return 1;
    }

    if (mkdir(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", table_name);
        return 2;
    }   

    path_db_table_meta(db_name, table_name, buffer, PATH_MAX);
    int meta_fd;
    int created_meta_file = create_file(buffer, &meta_fd);
    write_mt(schema, meta_fd);

    path_db_table_data(db_name, table_name, buffer, PATH_MAX);
    if (mkdir(buffer, 0755) != 0) {
        fprintf(stderr, "Failed to create a directory for table %s\n", table_name);
        return 3;
    }

    return 0;
}

int create_page(const char *db_name, const char *table_name, PageHeader *out_new_page) {
    uuid_generate_time(out_new_page->page_id);
    out_new_page->rows_count = 0;

    char file_path[PATH_MAX];
    path_db_table_page(db_name, table_name, out_new_page->page_id, file_path, PATH_MAX);

    int fd;
    int created_file = create_file(file_path, &fd);
    if (created_file != 0) {
        fprintf(stderr, "Failed to create page file %s\n", out_new_page->page_id);
        return 1;
    }

    write_ph(out_new_page, fd);
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
