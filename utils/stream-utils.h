#include <stdio.h>

#ifndef STREAM_UTILS_H
#define STREAM_UTILS_H

long fsize(FILE *file);
long fleft(FILE *file);
long fleftat(int pos, FILE *file);
int fmove(int pos, int offset, FILE *file);

#endif
