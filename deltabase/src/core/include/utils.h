#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

size_t
fsize(int fd);
size_t
fleft(int fd);
size_t
fleftat(size_t pos, int fd);
int
fmove(size_t pos, long int offset, int fd);

bool
dir_exists(const char* path);
int
mkdir_recursive(const char* path, mode_t mode);
int
rmdir_recursive(const char* path);

char**
get_dir_files(const char* dir_path, size_t* out_count);

char**
get_directories(const char* dir_path, size_t* out_count);

void
free_file_list(char** paths, size_t count);

char**
get_directory_names(const char* dir_path, size_t* out_count);

int
get_executable_dir(char* buffer, size_t buffer_size);


typedef enum LogLevel { LL_LOG = 1, LL_WARNING = 2, LL_ERROR = 3 } LogLevel;

void
dlt_log(LogLevel level, const char* format, ...);

#endif