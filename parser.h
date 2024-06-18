#ifndef PARSER_H
#define PARSER_H

struct Value *parse_expr(struct Parser *parser);
TopLevelDecls *parse_tlds(struct Parser *parser);

#endif

