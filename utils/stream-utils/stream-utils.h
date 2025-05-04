#include <stdio.h>

#ifndef STREAM_UTILS_H
#define STREAM_UTILS_H

long fsize(FILE *file);
long fleft(FILE *file);
long fleftat(size_t pos, FILE *file);
int fmove(size_t pos, long int offset, FILE *file);

#endif
