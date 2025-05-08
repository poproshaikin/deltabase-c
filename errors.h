#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

typedef enum ErrorCode {
    ERR_SUCCESS = 0,
    
    /* Invalid number literal */
    ERR_SQL_INV_NUM_LIT = 1,
    ERR_SQL_INV_STX_KW,
    ERR_SQL_UNEXP_SYM,

    ERR_INTRNL_TKN_INIT = 100
} ErrorCode;

typedef struct Error {
    size_t row;
    size_t symbol;
    ErrorCode error;
    // ErrorCode *errors;
    // int errors_count;
} Error;

Error create_error(size_t row, size_t symbol, ErrorCode code);
// Error *add_error(Error *error, ErrorCode new_code);
char *describe_error(Error error);

#endif
