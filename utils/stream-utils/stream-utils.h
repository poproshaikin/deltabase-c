#include <stdio.h>

#ifndef STREAM_UTILS_H
#define STREAM_UTILS_H

long fsize(FILE *file);
long fleft(FILE *file);
long fleftat(unsigned long int pos, FILE *file);
int fmove(unsigned long int pos, long int offset, FILE *file);

#endif
