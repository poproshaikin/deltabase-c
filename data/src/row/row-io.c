#include "../../data-storage.h"

#include <limits.h>
#include <stdlib.h>

#include "../token/token-io.h"
#include "../utils/utils.h"

/* Calculates null bitmap size in bytes */
static int nullbitmapsize(DataScheme *scheme) {
    return (scheme->columns_count + CHAR_BIT - 1) / CHAR_BIT;
}

/* Calculates an amount of null tokens in a null bitmap */
static int nullscount(unsigned char *nb, int len) {
    int count = 0;
    for (int i = 0; i < len; i++) {
        unsigned char byte = nb[i];
        for (int bit = 0; bit < 0; bit++) {
            if ((byte & (1 << bit)) == 1) {
                count++;
            }
        }
    }
    return count;
}

static unsigned char *buildnullbitmap(DataScheme *scheme, DataToken *tkns) {
    unsigned char *bitmap = calloc(nullbitmapsize(scheme), sizeof(char));

    for (int i = 0; i < scheme->columns_count; i++) {
        if (tkns[i].bytes == NULL) {
            int byteIndex = i / CHAR_BIT;
            int bitIndex = i % CHAR_BIT;
            bitmap[byteIndex] |= (1 << bitIndex);
        }
    }

    return bitmap;
}

/* Calculates a byte length of a row */
static dulen_t drowsize(DataScheme *scheme, DataToken *tokens, int tokensCount) {
    int nbSize = nullbitmapsize(scheme);
    dulen_t totalSize = nbSize;
    for (int i = 0; i < tokensCount; i++) {
        totalSize += dtoksize(&tokens[i]); 
    }
    return totalSize;
}

static unsigned char *readnullbitmap(DataScheme *scheme, FILE *file) {
    int nbSize = nullbitmapsize(scheme);
    unsigned char *bitmap = malloc(nbSize * sizeof(char));
    fread(bitmap, sizeof(char), nbSize, file);
    return bitmap;
}

int writedrow(DataScheme *scheme, DataToken *tokens, int tokens_count, FILE *file) {
    if (scheme == NULL || tokens == NULL || file == NULL) {
        return -1;
    }
    dulen_t rowSize = drowsize(scheme, tokens, tokens_count);
    fwrite(&rowSize, DT_LENGTH, 1, file);

    unsigned char *nb = buildnullbitmap(scheme, tokens);
    int nbSize = nullbitmapsize(scheme);
    fwrite(nb, sizeof(char), nbSize, file);

    for (int i = 0; i < tokens_count; i++) {
        writedtok(&tokens[i], file);
    }

    return 0;
}

DataToken **readdrow(DataScheme *scheme, FILE *file) {
    if (scheme == NULL || file == NULL) {
        return NULL;
    }

    dulen_t rowSize = readlenprefix(file);
    unsigned char *nb = readnullbitmap(scheme, file);
    DataToken **tokens = malloc(scheme->columns_count * sizeof(DataToken*));

    for (int i = 0; i < scheme->columns_count; i++) {
        DataToken *token = readdtok(file);
        if (token == NULL) {
            printf("Failed to read %i token", i);
            free(tokens);
            free(nb);
            return NULL;
        }
        tokens[i] = token;
    }

    return tokens;
}
