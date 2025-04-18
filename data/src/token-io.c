#include "../data-storage.h"

#include <stdio.h>
#include <stdlib.h>

#include "../../utils/stream-utils.h"

u_long write_token(DataToken *tkn, FILE *dest) {
    u_long written = fwrite(tkn->bytes, sizeof(char), tkn->size, dest);
    return written;
}

DataToken *read_token(DataTokenSize size, FILE *src) {
    char *buffer = malloc(sizeof(char) * size);
    if (buffer == NULL) {
        printf("Failed to allocate memory");
        free(buffer);
        return NULL;
    }

    int read = fread(buffer, sizeof(char), size, src);
    if (read < 0) {
        printf("Failed to read token. Size: %i. Pos: %li. Fd: %i\n", size, ftell(src), fileno(src));
        perror("Error");
        free(buffer);
        return NULL;
    }

    DataToken *tkn = malloc(sizeof(DataToken));
    if (tkn == NULL) {
        printf("Failed to allocate memory");
        free(buffer);
        return NULL;
    }

    tkn->size = size;
    tkn->bytes = buffer;
    return tkn;
}

int insert_token(DataToken *tkn, int pos, FILE *file) {
    fseek(file, pos, SEEK_SET);   
    int writingPos = fmove(pos, tkn->size, file);
    fseek(file, writingPos, SEEK_SET);
    write_token(tkn, file);
    return 0;
}
