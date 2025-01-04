#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "linkedlist.h"

struct Value *parse_expr(struct Globals *globals);
TopLevelDecls *parse_tlds(struct Globals *globals);

#endif

