#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Error create_error(size_t row, size_t symbol, ErrorCode code) {
    Error error = (Error){
        .row = row,
        .symbol = symbol,
        .error = code,
    };
    return error;
}

Error create_error_with_detail(size_t row, size_t symbol, ErrorCode code, char *detail) {
    Error error = create_error(row, symbol, code);
    error.detail = detail;
    return error;
}

static char *template(const char *message, Error error) {
    const int postfixLen = 64;
    char *result = malloc(strlen(message) + postfixLen);
    strcat(result, message);

    char positionPostfix[postfixLen];
    sprintf(positionPostfix, ": %lu:%lu\n", error.row, error.symbol);
    strcat(result, positionPostfix);
    return result;
}

char *describe_error(Error err) {
    switch (err.error) {
        case ERR_SQL_INV_STX_KW:
            return template("Syntax error", err);
        case ERR_SQL_INV_NUM_LIT:
            return template("Invalid number literal", err);
        case ERR_SQL_UNEXP_SYM:
            return template("Unexpected symbol", err);
        case ERR_SQL_UNEXP_TOK:
            return template("Unexpected token", err);
        case ERR_INTRNL_TKN_INIT:
            return template("INTERNAL: failed to initialize token", err);
        case ERR_INTRNL_MEM_ALLOC:
            return template("INTERNAL: memory allocation failed", err);
        case ERR_INTRNL_NOT_IMPL:
            return template("INTERNAL: this functionality is not implemented yet", err);
        case ERR_SUCCESS:
            return "No errors occured\n";
    }
    char *tmp = malloc(256);
    if (!tmp) {
        printf("Failed to allocate memory for a error string (xD)");
        return NULL;
    }

    int length = sprintf(tmp, "Error code: %i : %lu:%lu", err.error, err.row, err.symbol);

    tmp = realloc(tmp, length);
    if (!tmp) {
        printf("Failed to allocate memory for a error string (xD)");
        return NULL;
    }

    return tmp;
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
