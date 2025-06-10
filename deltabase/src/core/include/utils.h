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

int create_file(const char *path);

#endif
