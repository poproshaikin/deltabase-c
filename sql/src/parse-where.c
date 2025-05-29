#include "sql-parsing.h"
#include "sql-token.h"

AstNode *parse_expression(Parser *parser, Error *out_error) {
    
}

AstNode *parse_where(Parser *parser, Error *out_error) {
    if (!expect(parser, TT_KW_WHERE, out_error)) {
        return NULL;
    }

    AstNode *condition = parse_expression(parser, out_error);
    if (!condition) {
        printf("Expected condition after WHERE keyword\n");
        return NULL;
    }

}
