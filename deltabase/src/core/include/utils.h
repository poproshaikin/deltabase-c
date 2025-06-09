#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stdio.h>
#include <stdbool.h>

size_t fsize(FILE *file);
size_t fleft(FILE *file);
size_t fleftat(size_t pos, FILE *file);
int fmove(size_t pos, long int offset, FILE *file);

bool dir_exists(const char *path);

#endif
