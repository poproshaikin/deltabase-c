#include "sql-parsing.h"
#include "sql-lexing.h"
#include "sql-token.h"
#include <stdbool.h>
#include <stdlib.h>

AstNode *parse_select(Parser *parser, Error *out_error);

Parser create_parser(Token **tokens, size_t count) {
    return (Parser){
        .tokens = tokens,
        .count = count,
        .position = 0
    };
}

Token *current(Parser *parser) {
    if (parser->position > parser->count) {
        return NULL;
    }

    return parser->tokens[parser->position];
}

Token *previous(Parser *parser) {
    int newPos = parser->position - 1;
    if (newPos > parser->count) {
        return NULL;
    }
    
    return parser->tokens[newPos];
}

Token *next(Parser *parser) {
    int newPos = parser->position + 1;
    if (newPos > parser->count) {
        return NULL;
    }

    return parser->tokens[newPos];
}

bool advance(Parser *parser) {
    if (parser->position < parser->count) {
        parser->position++;
        return true;
    }
    return false;
}

bool match(Parser *parser, TokenType type) {
    Token *cur = current(parser);
    if (cur && cur->type == type) {
        advance(parser);
        return true;
    }
    return false;
}

bool expect(Parser *parser, TokenType type, Error *out_error) {
    if (!match(parser, type)) {
        if (out_error) {
            Token *cur = current(parser);
            char *detail = malloc(64);
            if (detail) {
                int len = sprintf(detail, "Expected token: %i\n", type);
                detail = realloc(detail, len);
                *out_error = create_error_with_detail(cur->row, cur->symbol, ERR_SQL_UNEXP_TOK, "Expected: %i");
            } else {
                *out_error = create_error(cur->row, cur->symbol, ERR_SQL_UNEXP_TOK);
            }
        } 
        advance(parser);
        return false;
    }
    return true;
}

AstNode *parse(Token **tokens, size_t count, Error *out_error) {
    if (!tokens || count == 0) 
        return NULL;

    Parser parser = create_parser(tokens, count);

    if (match(&parser, TT_KW_SELECT)) {
        return parse_select(&parser, out_error);
    }   

    if (out_error) 
        *out_error = create_error(0, 0, ERR_INTRNL_NOT_IMPL);

    return NULL;
}
