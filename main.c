#include "data/data-storage.h"
#include <stdio.h>
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

    char data[4] = { 1, 2, 3, 4 };
    DataToken token = {
        .size = TS_INTEGER,
        .bytes = data
    };

    /*u_long written = write_token(&token, file);   */
    /*move(0, 5, file);*/

    insert_token(&token, 2, file);

    /*DataToken *token = read_token(TS_INTEGER, file);*/
    /*int number = *(int*)token->bytes;*/
    /*printf("%i\n", number);*/

    /*char data[4] = { 1, 2, 3, 4 };*/
    /*DataToken token = {*/
    /*    .size = TS_INTEGER,*/
    /*    .bytes = data*/
    /*};*/
    /**/
    /*u_long written = write_token(&token, file);   */
    /*printf("%lu\n", written);*/

    fclose(file);
}
