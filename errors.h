#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

typedef enum ErrorCode {
    ERR_SUCCESS = 0,
    
    /* Invalid number literal */
    ERR_SQL_INV_NUM_LIT = 1,
    ERR_SQL_INV_STX_KW,
    ERR_SQL_UNEXP_SYM,
    ERR_SQL_UNEXP_TOK,

    ERR_INTRNL_TKN_INIT = 100,
    ERR_INTRNL_NOT_IMPL,
    ERR_INTRNL_MEM_ALLOC,
} ErrorCode;

typedef struct Error {
    size_t row;
    size_t symbol;
    ErrorCode error;
    char *detail;
} Error;

Error create_error(size_t row, size_t symbol, ErrorCode code);
Error create_error_with_detail(size_t row, size_t symbol, ErrorCode code, char *detail);
// Error *add_error(Error *error, ErrorCode new_code);
char *describe_error(Error error);

#endif
