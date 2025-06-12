#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

size_t fsize(int fd);
size_t fleft(int fd);
size_t fleftat(size_t pos, int fd);
int fmove(size_t pos, long int offset, int fd);

bool dir_exists(const char *path);
int mkdir_recursive(const char *path, mode_t mode);
int rmdir_recursive(const char *path);

char **get_dir_files(const char *dir_path, size_t *out_count);

#endif
