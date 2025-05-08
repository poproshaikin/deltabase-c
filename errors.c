#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Error create_error(size_t row, size_t symbol, ErrorCode code) {
    Error error = (Error){
        .row = row,
        .symbol = symbol,
        .error = code
    };
    return error;
}

static char *template(const char *message, size_t row, size_t symbol) {
    const int postfixLen = 64;
    char *result = malloc(strlen(message) + postfixLen);
    strcat(result, message);

    char positionPostfix[postfixLen];
    sprintf(positionPostfix, ": %lu:%lu\n", row, symbol);
    strcat(result, positionPostfix);
    return result;
}

char *describe_error(Error err) {
    switch (err.error) {
        case ERR_SQL_INV_STX_KW:
            return template("Syntax error", err.row, err.symbol);
        case ERR_SQL_INV_NUM_LIT:
            return template("Invalid number literal", err.row, err.symbol);
        case ERR_SQL_UNEXP_SYM:
            return template("Unexpected symbol", err.row, err.symbol);
        case ERR_INTRNL_TKN_INIT:
            return template("INTERNAL: failed to initialize token", err.row, err.symbol);
        case ERR_SUCCESS:
            return "No errors occured\n";
    }
    return "Unexpected error: no description provided\n";
}

// Error *add_error(Error *in_err, ErrorCode new_code) {
//     if (!in_err) {
//         Error *error = create_error()
//     }
//
//     ErrorCode *temp = realloc(in_err->errors, (in_err->errors_count + 1) * sizeof(ErrorCode));
//     if (!temp) {
//         perror("Failed to reallocate memory for new error array in add_error\n");
//         free(temp);
//         exit(1);
//     }
//     in_err->errors = temp;
//     in_err->errors_count++;
// }
