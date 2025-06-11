#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

size_t fsize(int fd);
size_t fleft(int fd);
size_t fleftat(size_t pos, int fd);
int fmove(size_t pos, long int offset, int fd);

bool dir_exists(const char *path);
int rmdir_recursive(const char *path);

// if OUT_FD is not null, leaves the created file opened and writes fd into it
// otherwise, closes the file
int create_file(const char *path, int *out_fd);

char **get_dir_files(const char *dir_path, size_t *out_count);

#endif
