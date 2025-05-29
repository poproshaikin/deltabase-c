#include "sql-ast.h"
#include "sql-parsing.h"
#include "sql-token.h"
#include <stdlib.h>

static void *handle_error(Error *out_error, ErrorCode code) {
    if (out_error) {
        *out_error = create_error(0, 0, code);
    }
    return NULL;
}

static void *handle_error_pos(size_t row, size_t symbol, Error *out_error, ErrorCode code) {
    if (out_error) {
        *out_error = create_error(row, symbol, code);
    }
    return NULL;
}

Token **parse_column_list(Parser *parser, size_t *out_count, Error *out_error) {
    Token **columns = malloc(parser->count * sizeof(Token*));
    size_t count = 0;

    while (true) {
        Token *cur = current(parser);

        if (!cur || cur->type == TT_KW_FROM || next(parser)->type == TT_KW_FROM) {
            break;
        }

        printf("token type: %i\n", cur->type);
        if (cur->type != TT_IDENTIFIER || cur->type != TT_OP_ASTERISK) {
            if (out_error) {
                *out_error = create_error(cur->row, cur->symbol, ERR_SQL_UNEXP_TOK);
            }
            free(columns);
            return NULL;
        }

        columns[count++] = cur;
        advance(parser);

        if (!match(parser, TT_SP_COMMA)) {
            break;
        }
    }
    
    columns = realloc(columns, count * sizeof(Token *));

    if (out_count) {
        *out_count = count;
    }

    return columns;
}

AstNode **create_columns_nodes(Token **tokens, size_t count, Error *out_error) {
    AstNode **nodes = malloc(count * sizeof(AstNode *));
    for (size_t i = 0; i < count; i++) {
        AstNode *column_node = create_node(out_error);
        if (!column_node) {
            for (size_t j = 0; j < i; j++) {
                free(nodes[j]);
            }
            free(nodes);
            return NULL;
        }
        column_node->type = NT_COLUMN_IDENTIFIER;
        column_node->value = tokens[i];

        nodes[i] = column_node;
    }
    return nodes;
}

AstNode *create_select_node(Token *table, Token **columns, size_t columns_count, Error *out_error) {
    AstNode *table_node = create_node(out_error);
    if (!table_node) {
        return NULL;
    }
    table_node->value = table;

    AstNode **columns_nodes = create_columns_nodes(columns, columns_count, out_error);
    if (!columns_nodes) {
        return NULL;
    }

    AstNode *select = create_node(out_error);
    if (!select) {
        return NULL;
    }
    select->select.columns_count = columns_count;
    select->select.columns = columns_nodes;
    select->select.table = table_node;
    
    return select;
}

AstNode *parse_select(Parser *parser, Error *out_error) {
    size_t columns_count = 0;
    Token **columns = parse_column_list(parser, &columns_count, out_error);

    if (!expect(parser, TT_KW_FROM, out_error)) {
        return NULL;
    }

    Token *table = current(parser);
    if (table->type != TT_IDENTIFIER) {
        return NULL;   
    }

    if (next(parser)->type == TT_KW_WHERE) {
        if (!out_error) {
            *out_error = create_error(next(parser)->row, next(parser)->symbol, ERR_INTRNL_NOT_IMPL); 
        }
        return NULL;
        //AstNode *where = parse_where(parser, out_error);
    }

    return create_select_node(table, columns, columns_count, out_error);
}
