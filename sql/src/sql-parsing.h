#ifndef SQL_PARSING_H
#define SQL_PARSING_H

#include "sql-ast.h"

typedef struct Parser {
    Token **tokens;
    size_t count;
    size_t position;
} Parser;

AstNode *parse(Token **tokens, size_t count, Error *out_error);

Parser create_parser(Token **tokens, size_t count);

// Returns a pointer to the token at the current parsing position.
// This does not modify the parser state.
Token *current(Parser *parser);

// Returns a pointer to the token immediately before the current parsing position.
// This does not modify the parser state.
Token *previous(Parser *parser);

// Returns a pointer to the token immediately after the current parsing position.
// This does not advance the parser position.
Token *next(Parser *parser);

// Advances the parser to the next token.
// Returns true if there are more tokens to parse, false otherwise.
bool advance(Parser *parser);

// Checks whether the token at the current parsing position matches the specified type.
// Returns true if it matches, false otherwise. 
// Advances the parser.
bool match(Parser *parser, TokenType type);

// Checks whether the token at the current parsing position matches the specified type.
// If it matches, advances the parser position and returns true.
// If it does not match, writes an error message to `out_error` and returns false.
bool expect(Parser *parser, TokenType type, Error *out_error);

Token **parse_column_list(Parser *parser, size_t *out_count, Error *out_error);

AstNode *parse_where(Parser *parser, Error *out_error);

#endif
