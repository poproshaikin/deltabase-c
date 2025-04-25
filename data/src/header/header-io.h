#include <stdio.h>
#include "../../data-storage.h"

#ifndef HEADER_IO_H
#define HEADER_IO_H

/* Writes a page header to file */
int writeph(PageHeader *header, FILE *file);

/* Reads a page header from file */
PageHeader *readph(FILE *file);

#endif
