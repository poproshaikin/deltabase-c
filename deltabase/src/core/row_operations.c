#include "include/core.h"
#include "include/data_io.h"
#include "include/data_page.h"
#include "include/data_table.h"
#include "include/path_service.h"
#include "include/utils.h"

#include <linux/limits.h>
#include <stdlib.h>

static int write_row_update_state(const DataRow *row, MetaTable *schema, FILE *file) {
    int fd = fileno(file);

    PageHeader header;
    read_page_header(file, &header);
    if (header.file_size >= MAX_PAGE_SIZE) {
        return 1;
    }

    header.rows_count++;
    header.file_size += dr_size(schema, row);
    header.max_rid = ++schema->last_rid;

    lseek(fd, 0, SEEK_SET);
    if (write_ph(&header, fd) != 0) {
        return 2;
    }

    lseek(fd, 0, SEEK_END);
    if (write_dr(schema, row, fd) != 0) {
        return 3;
    }

    return 0;
}

int insert_row(const char *db_name, const char *table_name, const DataRow *row) {
    char buffer[PATH_MAX];  
    path_db_table_data(db_name, table_name, buffer, PATH_MAX);

    MetaTable schema;
    if (get_table_schema(db_name, table_name, &schema) != 0) {
        return 1;
    }
    
    size_t pages_count;
    char **pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }

    for (size_t i = 0; i < pages_count; i++) {
        FILE *file = fopen(pages[i], "r+");
        if (!file) {
            fprintf(stderr, "Failed to open page for reading\n");
            return i + 3; // as the number of iteration it failed on + ensure that collision with two previous error codes wouldn't happen
        }

        int result = write_row_update_state(row, &schema, file);
        if (result == 1) { // means that the page is out of space
            continue;
        } else if (result > 1) { // something went wrong
            return i + 3; 
        } 

        fclose(file);
        break;
    }

    for (size_t i = 0; i < pages_count; i++) {
        free(pages[i]);
    }
    free(pages);

    return 0;
}

bool row_satisfies_filter(const DataRow *row, const DataFilter *filter) {
    
}

int update_row_by_filter(const char *db_name, const char *table_name, const DataFilter *filter, const DataRow *new_row) {
    char buffer[PATH_MAX];
    path_db_table_meta(db_name, table_name, buffer, PATH_MAX);

    MetaTable schema;
    if (get_table_schema(db_name, table_name, &schema) != 0) {
        return 1;
    }
    
    size_t pages_count;
    char **pages = get_dir_files(buffer, &pages_count);
    if (!pages) {
        return 2;
    }
    
    for (size_t i = 0; i < pages_count; i++) {
        FILE *file = fopen(pages[i], "r+");
        if (!file) {
            fprintf(stderr, "Failed to open page for reading\n");
            return i + 3; // as the number of iteration it failed on + ensure that collision with two previous error codes wouldn't happen
        }
        int fd = fileno(file);

        PageHeader header;
        if (read_ph(&header, fd) != 0) {
            return i + 3;
        }

        for (size_t j = 0; j < header.rows_count; j++) {
            DataRow row;
            if (read_dr(&schema, &row, fd) != 0) {

            }
        }
    }
}
