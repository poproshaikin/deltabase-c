#include "../../data-storage.h" 
#include <stdio.h>

#ifndef ROW_IO_H
#define ROW_IO_H

/* Writes a row of tokens to file */
int writedrow(DataScheme *scheme, DataToken *tkns, int tokens_count, FILE *file);

/* Reads a row of tokens from file */
DataToken **readdrow(DataScheme *scheme, FILE *file);

#endif
