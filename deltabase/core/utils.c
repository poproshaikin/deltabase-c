#include "include/utils.h"
#include <stdlib.h>
#include <unistd.h>

size_t fsize(FILE *file) {
    long currentPos = ftell(file);
    fseek(file, 0, SEEK_END);
    long end = ftell(file);
    fseek(file, currentPos, SEEK_SET);
    return end;
}

size_t fleftat(size_t pos, FILE *file) {
    fseek(file, 0, SEEK_END);
    size_t endPos = ftell(file);
    fseek(file, pos, SEEK_SET);
    return endPos - pos;
}

size_t fleft(FILE *file) {
    return fleftat(ftell(file), file);
}

static int fmove_pos(size_t pos, size_t offset, FILE *file) {
    long movingSize = fleftat(pos, file);

    char *moving = malloc(movingSize * sizeof(char));
    if (moving == NULL) {
        perror("Failed to allocate memory at fmove_pos");
        return -1;
    }
    fread(moving, sizeof(char), movingSize, file);

    char *filling = calloc(offset, sizeof(char));
    if (filling == NULL) {
        perror("Failed to allocate memory at fmove_pos");
        free(moving);
        return -1;
    }

    fseek(file, pos, SEEK_SET);
    unsigned long int fillStartIndex = ftell(file);
    fwrite(filling, sizeof(char), offset, file);
    fwrite(moving, sizeof(char), movingSize, file);

    free(moving);
    free(filling);

    return fillStartIndex;
}

static int fmove_neg(unsigned long int pos, long int offset, FILE *file) {
    int savingSize = pos + offset;
    int movingSize = fleftat(pos, file);
    char *movingbuffer = malloc(movingSize * sizeof(char));
    if (movingbuffer == NULL) {
        perror("Failed to allocate memory at fmove_neg");
        return -1;
    }

    fseek(file, pos, SEEK_SET);
    fread(movingbuffer, sizeof(char), movingSize, file);

    if (ftruncate(fileno(file), savingSize) != 0) {
        perror("Failed to truncate file in fmove_neg");
        return -1;
    }

    fseek(file, savingSize, SEEK_SET);
    fwrite(movingbuffer, sizeof(char), movingSize, file);

    free(movingbuffer);

    return movingSize;
}

int fmove(size_t pos, long int offset, FILE *file) {
    int finalOffset = offset;
    if (offset > 0) {
        return fmove_pos(pos, finalOffset, file);
    } 
    if (offset < 0) {
        return fmove_neg(pos, finalOffset, file);
    }
    return 0;
}
