#include "data/data-storage.h"
#include "data/src/row/row-io.h"
#include <stdlib.h>
#include "errors.h"
#include "sql/sql-engine.h"
#include "sql/src/sql-parsing.h"
#include "utils/utils.h"
#include <unistd.h>

// подготовить функции для работы с файлами.
// используя их начать разработку протокола и планирования структуры страниц.
// подготовить функции для работы с целыми страницами (создание, модификация, удаление)

// итого, сейчас я умею:
//   записать строку
//   удалить строку
//   переместить строку
//   записать заголовок 
//   прочитать заголовок
//   
// нужно:
//   структура страницы (решить, каждый файл под страницу, блоки, и тд)
//   выдавать ID строкам
//   пул открытых файлов
//   абстрагировать модификацию строк
// после:
//   оформить интерфейс взаимодействия с библиотекой
//   создать структуру директорий для таблиц
//   конфигурация таблиц
//
// алгоритм выполнения комманды с последующей модификацией FS:
//   получить комманду
//   обработать
//   составить план
//   использовать функции из data-storage.
//
// например:
//   SELECT * FROM Users;
//   найти папку с названием Users
//   найти все .rec файлы
//   асинхронно прочитать каждый файл
//     найти все файлы в условной /tables/Users
//     для каждого файла запустить поток
//     каждый поток обратится к пулу где создаст/получит открытый файл
//     ** конечно через mutex **
//     прочитает каждую строку
//   сгруппировать все массивы строк как нибудь
//      
//   совместить все в одну структуру/результат
//   вернуть

void hexdump(FILE *file) {
    rewind(file);

    unsigned char buffer[16];
    size_t bytesRead;

    printf("%lu\n", fleft(file));

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
}

int main(void) {
    printf("Enter command: ");

    char *str = NULL;
    size_t len = 0;

    ssize_t read = getline(&str, &len, stdin);
    if (read == -1) {
        return 1;
    }

    size_t count;
    Error err;
    Token **tokens = lex(str, &count, &err);

    for (int i = 0; i < count; i++) {
        printf("lexeme: %s, type: %i\n", tokens[i]->lexeme, tokens[i]->type);
    }

    AstNode *tree = parse(tokens, count, &err);

    if (err.error != 0) {
        printf("%s\n", describe_error(err));
        return 1;
    }

    printf("\n\n tree: %s\n", tree->select.table->value->lexeme);

    print_ast(tree, 4);
}

int maind(void) {

    FILE *file = fopen("hello.txt", "r+");
    if (file == NULL) {
        perror("fopen failed");
        return -1;
    }
    
    ftruncate(fileno(file), 0);

    /*DataToken token = {*/
    /*    .bytes = (char[]){'h', 'e', 'l', 'l', 'o'},*/
    /*    .size = 5,*/
    /*    .type = DT_STRING*/
    /*};*/
    /**/
    /*writedtok(&token, file);*/

    DataColumn col1 = { .name = "id", .data_type = DT_INTEGER, };
    DataColumn col2 = { .name = "name", .data_type = DT_STRING, };
    DataColumn col3 = { .name = "age", .data_type = DT_REAL, };

    DataColumn *columns[3] = { &col1, &col2, &col3 };

    DataScheme scheme = {
        .columns = columns,
        .columns_count = 3,
    };

    DataToken tokens[3] = {
        { .bytes = (char*)&(int){1}, .type = DT_INTEGER },
        { .size = 4, .bytes = (char[]){ 'i', 'v', 'a', 'n' }, .type = DT_STRING },
        { .bytes = (char*)&(double){18.5}, .type = DT_REAL },
    };

    writedrow(&scheme, tokens, 3, file);

    hexdump(file);

    /**/
    /*DataToken **tokens = readdrobool str_isint(const char *str) {
    // Если строка пуста, то это не целое число
    if (str == NULL || *str == '\0') {
        return false;
    }

    // Пропускаем возможный знак в начале
    if (*str == '+' || *str == '-') {
        str++;
    }

    // Проверяем, что после знака идут только цифры
    if (*str == '\0') {
        return false;  // Если после знака нет цифр, это не целое число
    }

    while (*str) {
        if (!isdigit(*str)) {
            return false;  // Если встречаем нецифровой символ, то строка не целое число
        }
        str++;
    }

    return true;  // Если строка состоит только из цифр (с учётом возможного знака), это целое число
}w(&scheme, file);*/
    /*if (tokens == NULL) {*/
    /*    printf("tokens are null\n");*/
    /*    return -1;*/
    /**/
    /*}*/
    /**/
    /*for (int i = 0; i < scheme.DataColumns_count; i++) {*/
    /*    printf("i: %i\n", i);*/
    /*    if (tokens[i]->bytes == NULL) {*/
    /*        printf("bytes are empty\n");*/
    /*    }*/
    /*    else if (scheme.DataColumns[i]->data_type == DT_INTEGER) {*/
    /*        printf("int: %i\n", *(int*)tokens[i]->bytes);*/
    /*    }*/
    /*    else if (scheme.DataColumns[i]->data_type == DT_STRING) {*/
    /*        printf("str: %s\n", tokens[i]->bytes);*/
    /*    }*/
    /*    else if (scheme.DataColumns[i]->data_type == DT_REAL) {*/
    /*        printf("real: %f", *(double*)tokens[i]->bytes);*/
    /*    }*/
    /*}*/

    /*PageHeader header = {*/
    /*    .page_id = 1,*/
    /*    .free_rows = (toklen_t[]){ 1, 4, 7 },*/
    /*    .free_rows_count = 3,*/
    /*    .rows_count = 15,*/
    /*};*/
    /**/
    /*writeph(&header, file);*/

    /**/
    /*PageHeader *header = readph(file);  */
    /*if (header == NULL) {*/
    /*    return 1;*/
    /*}*/
    /*printf("Page id: %lu\n", header->page_id);*/
    /*printf("Rows count: %lu\n", header->rows_count);*/
    /*printf("Free rows count: %lu\n", header->free_rows_count);*/


    fclose(file);
}

