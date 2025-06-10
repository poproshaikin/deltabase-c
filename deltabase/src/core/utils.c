#include "include/utils.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

size_t fsize(int fd) {
    long currentPos = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_END);
    long end = lseek(fd, 0, SEEK_CUR);
    lseek(fd, currentPos, SEEK_SET);
    return end;
}

size_t fleftat(size_t pos, int fd) {
    lseek(fd, 0, SEEK_END);
    size_t endPos = lseek(fd, 0, SEEK_CUR);
    lseek(fd, pos, SEEK_SET);
    return endPos - pos;
}

size_t fleft(int fd) {
    return fleftat(lseek(fd, 0, SEEK_CUR), fd);
}

static int fmove_pos(size_t pos, size_t offset, int fd) {
    long movingSize = fleftat(pos, fd);

    char *moving = malloc(movingSize * sizeof(char));
    if (moving == NULL) {
        perror("Failed to allocate memory at fmove_pos");
        return -1;
    }
    read(fd, moving, movingSize);

    char *filling = calloc(offset, sizeof(char));
    if (filling == NULL) {
        perror("Failed to allocate memory at fmove_pos");
        free(moving);
        return -1;
    }

    lseek(fd, pos, SEEK_SET);
    unsigned long int fillStartIndex = lseek(fd, 0, SEEK_CUR);
    write(fd, filling, offset);
    write(fd, moving, movingSize);

    free(moving);
    free(filling);

    return fillStartIndex;
}

static int fmove_neg(unsigned long int pos, long int offset, int fd) {
    int savingSize = pos + offset;
    int movingSize = fleftat(pos, fd);
    char *movingbuffer = malloc(movingSize * sizeof(char));
    if (movingbuffer == NULL) {
        perror("Failed to allocate memory at fmove_neg");
        return -1;
    }

    lseek(fd, pos, SEEK_SET);
    read(fd, movingbuffer, movingSize);

    if (ftruncate(fd, savingSize) != 0) {
        perror("Failed to truncate fd in fmove_neg");
        return -1;
    }

    lseek(fd, savingSize, SEEK_SET);
    write(fd, movingbuffer, movingSize);

    free(movingbuffer);

    return movingSize;
}

int fmove(size_t pos, long int offset, int fd) {
    int finalOffset = offset;
    if (offset > 0) {
        return fmove_pos(pos, finalOffset, fd);
    } 
    if (offset < 0) {
        return fmove_neg(pos, finalOffset, fd);
    }
    return 0;
}

bool dir_exists(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

int rmdir_recursive(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = 0; 

    if (!d) {
        perror("remove_directory_recursive: opendir");
        return -1;
    }

    struct dirent *p;
    while ((p = readdir(d)) != NULL) {
        if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
            continue;
        }

        size_t len = path_len + strlen(p->d_name) + 2;
        char *buf = malloc(len);
        if (!buf) {
            perror("remove_directory_recursive: malloc");
            r = -1; 
            break;  
        }

        snprintf(buf, len, "%s/%s", path, p->d_name);

        struct stat statbuf;
        if (stat(buf, &statbuf) == -1) {
            perror("remove_directory_recursive: stat");
            free(buf);
            r = -1; 
            break;  
        }

        if (S_ISDIR(statbuf.st_mode)) {
            if (rmdir_recursive(buf) == -1) {
                free(buf);
                r = -1; 
                break; 
            }
        } else {
            if (remove(buf) == -1) {
                perror("remove_directory_recursive: remove file");
                free(buf);
                r = -1;
                break;
            }
        }
        free(buf); 
    }

    
    closedir(d);

        if (r == 0) {
        if (rmdir(path) == -1) {
            perror("remove_directory_recursive: rmdir");
            r = -1; 
        }
    }

    return r;
}

int create_file(const char *path) {
    FILE *tables_meta = fopen(path, "w");
    if (!tables_meta) {
        fprintf(stderr, "Failed to create file\n");
        return 1;
    }
    fclose(tables_meta);
    return 0;
}
