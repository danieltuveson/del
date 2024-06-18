#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "linkedlist.h"

struct Parser {
    struct LinkedListNode *head;
    struct Lexer *lexer;
};

struct Value *parse_expr(struct Parser *parser);
TopLevelDecls *parse_tlds(struct Parser *parser);

#endif

