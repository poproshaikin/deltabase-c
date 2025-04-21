#include "data/data-storage.h"
#include <stdlib.h>

// подготовить функции для работы с файлами.
// используя их начать разработку протокола и планирования структуры страниц.
// подготовить функции для работы с целыми страницами (создание, модификация, удаление)

int main(void) {
    FILE *file = fopen("hello.txt", "r+");
    if (file == NULL) {
        perror("fopen failed");
        return -1;
    }

    /*Column col1 = { .name = "id", .data_type = DT_INTEGER, .size = DTS_INTEGER };*/
    /*Column col2 = { .name = "name", .data_type = DT_STRING };*/
    /*Column col3 = { .name = "age", .data_type = DT_REAL, .size = DTS_REAL };*/
    /**/
    /*Column *columns[3] = { &col1, &col2, &col3 };*/
    /**/
    /*DataScheme scheme = {*/
    /*    .columns = columns,*/
    /*    .columns_count = 3,*/
    /*};*/
    /**/
    /*DataToken **tokens = readdrow(&scheme, file);*/
    /*if (tokens == NULL) {*/
    /*    printf("tokens are null\n");*/
    /*    return -1;*/
    /**/
    /*}*/
    /**/
    /*for (int i = 0; i < scheme.columns_count; i++) {*/
    /*    printf("i: %i\n", i);*/
    /*    if (tokens[i]->bytes == NULL) {*/
    /*        printf("bytes are empty\n");*/
    /*    }*/
    /*    else if (scheme.columns[i]->data_type == DT_INTEGER) {*/
    /*        printf("int: %i\n", *(int*)tokens[i]->bytes);*/
    /*    }*/
    /*    else if (scheme.columns[i]->data_type == DT_STRING) {*/
    /*        printf("str: %s\n", tokens[i]->bytes);*/
    /*    }*/
    /*    else if (scheme.columns[i]->data_type == DT_REAL) {*/
    /*        printf("real: %f", *(double*)tokens[i]->bytes);*/
    /*    }*/
    /*}*/

    /*PageHeader header = {*/
    /*    .page_id = 1,*/
    /*    .free_rows = (int[]){ 1, 4, 7 },*/
    /*    .free_rows_count = 3,*/
    /*    .rows_count = 15,*/
    /*};*/
    /**/
    /*writeph(&header, file);*/

    PageHeader *header = readph(file);  
    printf("Page id: %i\n", header->page_id);
    printf("Rows count: %i\n", header->rows_count);
    printf("Free rows count: %i\n", header->free_rows_count);

    /*DataToken tokens[3] = {*/
    /*    { .size = 4, .bytes = (char*)&(int){1}, .type = DT_INTEGER },*/
    /*    { .size = 4, .bytes = (char[]){ 'i', 'v', 'a', 'n' }, .type = DT_STRING },*/
    /*    { .size = 8, .bytes = (char*)&(double){18.5}, .type = DT_REAL },*/
    /*};*/
    /*writedrow(&scheme, tokens, file);*/

    fclose(file);
}
