#include <cstdio>
#include <cstring>
#include <iostream>
extern "C" {
    #include "../src/core/include/data_io.h"
    #include "../src/core/include/data_token.h"
}

using namespace std;

int main() {
    FILE *file = fopen("hello.txt", "r+");

    /*
    int i = 42;
    double d = 3.14;
    char c = 'A';
    bool b = true;
    const char *str = "Hello";

    DataToken *tokens[5];
    tokens[0] = make_token(DT_INTEGER, &i, sizeof(int));
    tokens[1] = make_token(DT_REAL, &d, sizeof(double));
    tokens[2] = make_token(DT_CHAR, &c, sizeof(char));
    tokens[3] = make_token(DT_BOOL, &b, sizeof(bool));
    tokens[4] = make_token(DT_STRING, str, strlen(str) + 1);   

    DataSchema schema = {
        .columns_count = 5
    };

    std::cout << "writing";
    int result = write_dr_v(&schema, tokens, 5, 0, file);
    std::cout << "writed";
    */

    DataSchema schema = {
        .columns_count = 5
    };

    DataRow row;
    read_dr(&schema, &row, fileno(file));

    for (int i = 0; i < row.count; i++) {
        DataToken* tok = row.tokens[i];
        switch (tok->type) {
            case DT_STRING:
                std::cout << std::string(tok->bytes, tok->size) << std::endl;
                break;
            case DT_INTEGER: 
                int val;
                std::memcpy(&val, tok->bytes, sizeof(int));
                std::cout << val << std::endl;
                break;
                             
            case DT_REAL: {
                double val;
                std::memcpy(&val, tok->bytes, sizeof(double));
                std::cout << val << std::endl;
                break;
            }
            case DT_BOOL: {
                bool val;
                std::memcpy(&val, tok->bytes, sizeof(bool));
                std::cout << (val ? "true" : "false") << std::endl;
                break;
            }
            case DT_CHAR:
                std::cout << *tok->bytes << std::endl;
                break;
            default:
                std::cout << "<unknown>" << std::endl;
        }
    }

    fclose(file);

    return 0;
}
