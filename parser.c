#include "common.h"
#include "linkedlist.h"
#include "lexer.h"
#include "error.h"
#include "ast.h"
#include "printers.h"
#include "parser.h"

// TODO: improve quality of error messages

typedef struct Value *(*ExprParserFunction)(struct Parser *);
typedef struct Expr *(*ExprFunction)(struct Value *, struct Value *);

struct ExprPair {
    enum TokenType type;
    ExprFunction efunc;
};

// Forward declarations
static struct Value *parse_subexpr(struct Parser *parser);
static struct Statement *parse_statement(struct Parser *parser);

static inline bool parser_peek(struct Parser *parser, enum TokenType match)
{
    if (parser->head == NULL) {
        return false;
    }
    struct Token *token = parser->head->value;
    return token->type == match;
}

// Parses one or more tokens on success, does not consume tokens on failure
static bool parser_match(struct Parser *parser, enum TokenType *matches, size_t matches_length)
{
    if (parser->head == NULL) {
        return false;
    }
    struct LinkedListNode *head = parser->head;
    size_t i = 0;
    linkedlist_foreach(lnode, head) {
        if (i >= matches_length) {
            break;
        }
        struct Token *token = lnode->value;
        if (token->type != matches[i]) {
            return false;
        }
        i++;
        head = lnode;
    }
    if (i < matches_length) {
        return false;
    }
    parser->head = head->next;
    return true;
}

#define match_multiple(parser, matches)\
        parser_match(parser, matches, sizeof(matches) / sizeof(*matches))

static inline bool match(struct Parser *parser, enum TokenType match)
{
    return parser_match(parser, &match, 1);
}

static struct LinkedListNode *nth_lnode(struct LinkedListNode *head, int num)
{
    int i = 1; // starting index at 1 to match flex/bison
    linkedlist_foreach(lnode, head) {
        if (i == num) {
            return lnode;
        }
        i++;
    }
    return NULL;
}

static inline struct Token *nth_token(struct LinkedListNode *head, int num)
{
    struct LinkedListNode *lnode = nth_lnode(head, num);
    return lnode == NULL ? NULL : lnode->value;
}

// Cursed helper function for parsing expressions.
// I would have had to write basically the same code for every binary expression, but instead I'm
// using function pointers to DRY it up. I know it looks heinous.
static struct Value *parse_binexp_help(struct Parser *parser, ExprParserFunction epfunc,
                                       struct ExprPair *epairs, size_t epairs_length)
{
    struct Value *left = epfunc(parser);
    if (left == NULL) {
        return NULL;
    }
    struct Value *right = NULL;
    while (parser->head != NULL) {
        for (size_t i = 0; i < epairs_length; i++) {
            enum TokenType type = epairs[i].type;
            ExprFunction efunc = epairs[i].efunc;
            if (match(parser, type)) {
                right = epfunc(parser);
                if (right == NULL) {
                    return NULL;
                }
                left = new_expr(efunc(left, right));
                goto continue_while;
            }
        }
        break;
        continue_while:
        continue;
    }
    return left;
}

#define parse_binexp(parser, epfunc, epairs) \
    parse_binexp_help(parser, epfunc, epairs, sizeof(epairs) / sizeof(*epairs))

static struct ExprPair orexpr_pairs[] = {
    { ST_OR, bin_or },
};

static struct ExprPair andexpr_pairs[] = {
    { ST_AND, bin_and },
};

static struct ExprPair eqexpr_pairs[] = {
    { ST_EQEQ, bin_eqeq }, { ST_NOT_EQ, bin_not_eq },
};

static struct ExprPair compexpr_pairs[] = {
    { ST_GREATER, bin_greater }, { ST_GREATER_EQ, bin_greater_eq },
    { ST_LESS,    bin_less    }, { ST_LESS_EQ,    bin_less_eq    },
};

static struct ExprPair addexpr_pairs[] = {
    { ST_PLUS,  bin_plus }, { ST_MINUS, bin_minus },
};

static struct ExprPair multexpr_pairs[] = {
    { ST_STAR,  bin_star }, { ST_SLASH, bin_slash }, { ST_PERCENT, bin_percent },
};

static inline struct Value *parse_multexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_subexpr, multexpr_pairs);
}

static inline struct Value *parse_addexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_multexpr, addexpr_pairs);
}

static inline struct Value *parse_compexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_addexpr, compexpr_pairs);
}

static inline struct Value *parse_eqexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_compexpr, eqexpr_pairs);
}

static inline struct Value *parse_andexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_eqexpr, andexpr_pairs);
}

static inline struct Value *parse_orexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_andexpr, orexpr_pairs);
}

struct Value *parse_expr(struct Parser *parser)
{
    return parse_orexpr(parser);
}

static Values *parse_args(struct Parser *parser)
{
    struct Value *val = NULL;
    Values *vals = linkedlist_new();
    do {
        if ((val = parse_expr(parser))) {
            linkedlist_append(vals, val);
        } else {
            return NULL;
        }
    } while (match(parser, ST_COMMA));
    if (!match(parser, ST_CLOSE_PAREN)) {
        error_parser("Unexpected end of argument list");
        return NULL;
    }
    return vals;
}

#define parse_call(rettype, parser, symbol, func, builtin) \
    do {                                          \
    Values *vals = NULL;                          \
    if (match(parser, ST_CLOSE_PAREN)) {          \
        rettype = func(symbol, NULL, builtin); \
    } else if ((vals = parse_args(parser))) {     \
        rettype = func(symbol, vals, builtin); \
    } else {                                      \
        rettype = NULL;                           \
    }                                             \
    } while (false)

static struct Value *parse_vfuncall(struct Parser *parser, Symbol symbol,
    struct Value *func(Symbol, Values *, bool))
{
    struct Value *val;
    bool builtin = is_builtin(symbol);
    parse_call(val, parser, symbol, func, builtin);
    return val;
}

static struct Value *parse_get(struct Parser *parser, Symbol symbol)
{
    struct LinkedList *lvalues = linkedlist_new();
    while (true) {
        if (match(parser, ST_DOT)) {
            struct LinkedListNode *old_head = parser->head;
            if (!match(parser, T_SYMBOL)) {
                error_parser("Expected property or method name");
                return NULL;
            }
            Symbol prop = nth_token(old_head, 1)->symbol;
            linkedlist_append(lvalues, new_property(prop));
        } else if (match(parser, ST_OPEN_BRACKET)) {
            struct Value *expr = parse_expr(parser);
            if (expr == NULL) {
                return NULL;
            } else if (!match(parser, ST_CLOSE_BRACKET)) {
                error_parser("Expected closing bracket in accessor");
                return NULL;
            }
            linkedlist_append(lvalues, new_index(expr));
        } else {
            break;
        }
    }
    return new_get(symbol, lvalues);
}

// TODO: Add options for method call
static struct Value *parse_subexpr(struct Parser *parser)
{
    if (parser->head == NULL) {
        error_parser("Unexpected end of expression");
        return NULL;
    }
    struct Value *val = NULL;
    struct LinkedListNode *old_head = parser->head;
    if (match(parser, T_INT)) {
        return new_integer(nth_token(old_head, 1)->integer);
    } else if (match(parser, T_FLOAT)) {
        // TODO: parse float
        assert("TODO: parse float" && false);
    } else if (match(parser, T_STRING)) {
        return new_string(nth_token(old_head, 1)->string);
    } else if (match(parser, ST_TRUE)) {
        return new_boolean(1);
    } else if (match(parser, ST_FALSE)) {
        return new_boolean(0);
    } else if (match(parser, ST_NULL)) {
        return new_null();
    } else if (match(parser, ST_PLUS) && (val = parse_subexpr(parser))) {
        return new_expr(unary_plus(val));
    } else if (match(parser, ST_MINUS) && (val = parse_subexpr(parser))) {
        return new_expr(unary_minus(val));
    } else if (match(parser, ST_OPEN_PAREN) && (val = parse_expr(parser))
            && match(parser, ST_CLOSE_PAREN)) {
        return val;
    } else if (match(parser, T_SYMBOL)) {
        Symbol symbol = nth_token(old_head, 1)->symbol;
        if (match(parser, ST_OPEN_PAREN)) {
            return parse_vfuncall(parser, symbol, new_vfuncall);
        // } else if (parser_peek(parser, ST_DOT) || parser_peek(parser, ST_OPEN_BRACKET)) {
        } else { // if (parser_peek(parser, ST_DOT) || parser_peek(parser, ST_OPEN_BRACKET)) {
            return parse_get(parser, symbol);
        }
        // } else {
        //     return new_symbol(symbol);
        // }
    } else if (match(parser, ST_NEW) && match(parser, T_SYMBOL) && match(parser, ST_OPEN_PAREN)) {
        return parse_vfuncall(parser, nth_token(old_head, 2)->symbol, new_constructor);
    }
    error_parser("Unexpected token in expression");
    return NULL;
}

static struct Statement *parse_sfuncall(struct Parser *parser, Symbol symbol,
    struct Statement *func(Symbol, Values *, bool))
{
    struct Statement *stmt;
    bool builtin = is_builtin(symbol);
    parse_call(stmt, parser, symbol, func, builtin);
    return stmt;
}

static struct Statement *parse_set(struct Parser* parser, Symbol symbol, LValues *lvalues,
    bool is_define)
{
    struct Value *expr;
    if ((expr = parse_expr(parser))) {
        return new_set(symbol, expr, lvalues, is_define);
    }
    return NULL;
}

static Definitions *parse_symbols(struct Parser *parser)
{
    Definitions *defs = linkedlist_new();
    do {
        struct LinkedListNode *old_head = parser->head;
        if (match(parser, T_SYMBOL)) {
            Symbol symbol = nth_token(old_head, 1)->symbol;
            linkedlist_append(defs, new_define(symbol, TYPE_UNDEFINED));
        } else {
            error_parser("Unexpected token in definition");
            return NULL;
        }
    } while (match(parser, ST_COMMA));
    return defs;
}

// accessors: accessor accessors { $$ = append($2, $1); }
//          | accessor ST_EQ { $$ = new_list($1); }
// 
// accessor: ST_DOT T_SYMBOL { $$ = new_property($2); }
//         | ST_OPEN_BRACKET T_INT ST_CLOSE_BRACKET { $$ = new_index(new_integer($2)); }
// TODO: Add options for method call
static struct Statement *parse_lhs(struct Parser *parser, Symbol symbol)
{
    struct LinkedList *lvalues = linkedlist_new();
    while (true) {
        if (match(parser, ST_EQ)) {
            return parse_set(parser, symbol, lvalues, false);
        } else if (match(parser, ST_OPEN_PAREN)) {
            // assert("TODO: update parse_sfuncall to accept accessors" && false);
            // return NULL;
            return parse_sfuncall(parser, symbol, new_sfuncall);
        }
        if (match(parser, ST_DOT)) {
            struct LinkedListNode *old_head = parser->head;
            if (!match(parser, T_SYMBOL)) {
                error_parser("Expected property or method name");
                return NULL;
            }
            Symbol prop = nth_token(old_head, 1)->symbol;
            linkedlist_append(lvalues, new_property(prop));
        } else if (match(parser, ST_OPEN_BRACKET)) {
            struct Value *expr = parse_expr(parser);
            if (expr == NULL) {
                return NULL;
            } else if (!match(parser, ST_CLOSE_BRACKET)) {
                error_parser("Expected closing bracket in accessor");
                return NULL;
            }
            linkedlist_append(lvalues, new_index(expr));
        } else {
            break;
        }
    }
    error_parser("Unexpected value in accessor");
    return NULL;
}

static struct Statement *parse_line(struct Parser *parser)
{
    if (parser->head == NULL) {
        error_parser("Unexpected end of input. Expected start of statement");
        return NULL;
    }
    struct LinkedListNode *old_head = parser->head;
    if (match(parser, T_SYMBOL)) {
        Symbol symbol = nth_token(old_head, 1)->symbol;
        return parse_lhs(parser, symbol);
    } else if (match(parser, ST_LET)) {
        Definitions *defs = NULL;
        enum TokenType matches[] = { T_SYMBOL, ST_EQ };
        if (match_multiple(parser, matches)) {
            Symbol symbol = nth_token(old_head, 2)->symbol;
            return parse_set(parser, symbol, linkedlist_new(), true);
        } else if ((defs = parse_symbols(parser))) {
            return new_let(defs);
        }
        return NULL;
    } else if (match(parser, ST_RETURN)) {
        struct Value *expr = NULL;
        if (parser_peek(parser, ST_SEMICOLON)) {
            return new_return(NULL);
        } else if ((expr = parse_expr(parser))) {
            return new_return(expr);
        }
        return NULL;
    }
    error_parser("Unexpected token in statement");
    return NULL;
}

static Statements *parse_block(struct Parser *parser, char *type)
{
    if (!match(parser, ST_OPEN_BRACE)) {
        error_parser("%s missing opening brace", type);
        return NULL;
    }
    struct Statement *stmt = NULL;
    Statements *stmts = linkedlist_new();
    while (!match(parser, ST_CLOSE_BRACE)) {
        stmt = parse_statement(parser);
        if (!stmt) {
            return NULL;
        }
        linkedlist_append(stmts, stmt);
    }
    return stmts;
}

static struct Statement *parse_if_branch(struct Parser *parser, char *type)
{
    struct Value *condition = parse_expr(parser);
    if (!condition) {
        return NULL;
    }
    Statements *if_stmts = parse_block(parser, type);
    if (!if_stmts) {
        return NULL;
    }
    return new_if(condition, if_stmts);
}

static struct Statement *parse_if(struct Parser *parser)
{
    struct Statement *first_if_stmt = parse_if_branch(parser, "if block");
    if (!first_if_stmt) {
        return NULL;
    }
    struct Statement *stmt = first_if_stmt;
    enum TokenType elseif[] = { ST_ELSE, ST_IF };
    while (match_multiple(parser, elseif)) {
        struct Statement *elseif_stmt = parse_if_branch(parser, "else if block");
        if (!elseif_stmt) {
            return NULL;
        }
        stmt = add_elseif(stmt->if_stmt, elseif_stmt);
    }
    if (match(parser, ST_ELSE)) {
        Statements *else_stmts = parse_block(parser, "else block");
        if (!else_stmts) {
            return NULL;
        }
        add_else(stmt->if_stmt, else_stmts);
    }
    return first_if_stmt;
}

static struct Statement *parse_statement(struct Parser *parser)
{
    struct Value *expr = NULL;
    struct Statement *stmt = NULL;
    Statements *stmts = NULL;
    if (match(parser, ST_IF)) {
        if ((stmt = parse_if(parser))) {
            return stmt;
        }
        return NULL;
    } else if (match(parser, ST_FOR)) {
        struct Statement *init, *increment;
        // Parens are allowed to surround for loop arguments but are not required
        bool redundant_paren = match(parser, ST_OPEN_PAREN);
        if ((init = parse_line(parser)) && match(parser, ST_SEMICOLON)
                && (expr = parse_expr(parser)) && match(parser, ST_SEMICOLON) 
                && (increment = parse_line(parser))) {
            if (redundant_paren != match(parser, ST_CLOSE_PAREN)) {
                error_parser("mismatched parenthesis");
                return NULL;
            } else if ((stmts = parse_block(parser, "for block"))) {
                return new_for(init, expr, increment, stmts);
            }
        }
        return NULL;
    } else if (match(parser, ST_WHILE)) {
        if ((expr = parse_expr(parser))) {
            if ((stmts = parse_block(parser, "while block"))) {
                return new_while(expr, stmts);
            }
        }
        return NULL;
    } else if ((stmt = parse_line(parser))) {
        if (match(parser, ST_SEMICOLON)) {
            return stmt;
        }
        error_parser("Unexpected end of statement");
    } 
    return NULL;
}

// type: ST_INT { $$ = TYPE_INT; }
//     | ST_FLOAT { $$ = TYPE_FLOAT; }
//     | ST_BOOL { $$ = TYPE_BOOL; }
//     | ST_STRING { $$ = TYPE_STRING; }
//     | T_SYMBOL { $$ = $1; }
// ;
static Type parse_type(struct Parser *parser)
{
    struct LinkedListNode *old_head = parser->head;
    if (match(parser, ST_INT)) {
        return TYPE_INT;
    } else if (match(parser, ST_FLOAT)) {
        return TYPE_FLOAT;
    } else if (match(parser, ST_BOOL)) {
        return TYPE_BOOL;
    } else if (match(parser, ST_STRING)) {
        return TYPE_STRING;
    } else if (match(parser, T_SYMBOL)) {
        return nth_token(old_head, 1)->symbol;
    }
    error_parser("invalid type");
    return TYPE_UNDEFINED;
}

// definitions: definition { $$ = new_list($1); }
//            | definition ST_COMMA definitions { $$ = append($3, $1); }
// ;
// 
// definition: T_SYMBOL ST_COLON type { $$ = new_define($1, $3); };
static struct Definition *parse_definition(struct Parser *parser)
{
    struct LinkedListNode *old_head = parser->head;
    Type type = TYPE_UNDEFINED;
    if (!match(parser, T_SYMBOL)) {
        error_parser("expected variable name");
        return NULL;
    } else if (match(parser, ST_COMMA) || match(parser, ST_CLOSE_PAREN)) {
        error_parser("function argument is missing a type");
        return NULL;
    } else if (!(match(parser, ST_COLON))) {
        error_parser("expected colon");
        return NULL;
    } else if ((type = parse_type(parser)) == TYPE_UNDEFINED) {
        error_parser("invalid type");
        return NULL;
    }
    return new_define(nth_token(old_head, 1)->symbol, type);
}

// fundef_args: ST_OPEN_PAREN definitions ST_CLOSE_PAREN { $$ = $2; }
//            | ST_OPEN_PAREN ST_CLOSE_PAREN { $$ = NULL; }
// ;
static Definitions *parse_fundef_args(struct Parser *parser)
{
    if (!match(parser, ST_OPEN_PAREN)) {
        error_parser("expected list of function arguments");
        return NULL;
    }
    Definitions *definitions = linkedlist_new();
    if (match(parser, ST_CLOSE_PAREN)) {
        return definitions;
    }
    do {
        struct Definition *definition = parse_definition(parser);
        if (!definition) {
            return NULL;
        }
        linkedlist_append(definitions, definition);
    } while (match(parser, ST_COMMA));
    if (!match(parser, ST_CLOSE_PAREN)) {
        error_parser("Unexpected end of argument list");
        return NULL;
    }
    return definitions;
}

// fundef: ST_FUNCTION T_SYMBOL fundef_args ST_COLON type ST_OPEN_BRACE statements ST_CLOSE_BRACE
//       { $$ = new_tld_fundef($2, $5, $3, $7); }
//       | ST_FUNCTION T_SYMBOL fundef_args ST_OPEN_BRACE statements ST_CLOSE_BRACE
// { $$ = new_tld_fundef($2, TYPE_UNDEFINED, $3, $5); }
// ;
static struct TopLevelDecl *parse_fundef(struct Parser *parser)
{
    struct LinkedListNode *old_head = parser->head;
    if (match(parser, T_SYMBOL)) {
        Symbol funname = nth_token(old_head, 1)->symbol;
        if (is_builtin(funname)) {
            error_parser("cannot use builtin function name as declaration");
            return NULL;
        }
        Definitions *defs = parse_fundef_args(parser);
        Type rettype = TYPE_UNDEFINED;
        if (!defs) {
            return NULL;
        } else if (match(parser, ST_COLON)) {
            rettype = parse_type(parser);
            if (rettype == TYPE_UNDEFINED) {
                return NULL;
            }
        }
        Statements *stmts = parse_block(parser, lookup_symbol(funname));
        if (!stmts) {
            return NULL;
        }
        return new_tld_fundef(funname, rettype, defs, stmts);
    }
    error_parser("could not parse function");
    return NULL;
}

static struct Definition *parse_class_definition(struct Parser *parser)
{
    struct LinkedListNode *old_head = parser->head;
    Type type = TYPE_UNDEFINED;
    if (!match(parser, T_SYMBOL)) {
        error_parser("expected property name");
        return NULL;
    } else if (match(parser, ST_SEMICOLON)) {
        error_parser("class property is missing a type");
        return NULL;
    } else if (!(match(parser, ST_COLON))) {
        error_parser("expected colon");
        return NULL;
    } else if ((type = parse_type(parser)) == TYPE_UNDEFINED) {
        error_parser("invalid type");
        return NULL;
    }
    return new_define(nth_token(old_head, 1)->symbol, type);
}

// class_definitions: definition ST_SEMICOLON { $$ = new_list($1); }
//                  | definition ST_SEMICOLON class_definitions { $$ = append($3, $1); }
// ;
static Definitions *parse_class_defs(struct Parser *parser)
{
    if (!match(parser, ST_OPEN_BRACE)) {
        error_parser("expected list of class properties");
        return NULL;
    }
    Definitions *definitions = linkedlist_new();
    do {
        if (match(parser, ST_CLOSE_BRACE)) {
            return definitions;
        }
        struct Definition *definition = parse_class_definition(parser);
        if (!definition) {
            return NULL;
        }
        linkedlist_append(definitions, definition);
    } while (match(parser, ST_SEMICOLON));
    if (!match(parser, ST_CLOSE_BRACE)) {
        error_parser("Unexpected end of property list");
        return NULL;
    }
    return definitions;
}

// cls: ST_CLASS T_SYMBOL ST_OPEN_BRACE class_definitions methods ST_CLOSE_BRACE
//        { $$ = new_class($2, $4, $5); }
//    | ST_CLASS T_SYMBOL ST_OPEN_BRACE class_definitions ST_CLOSE_BRACE
//        { $$ = new_class($2, $4, NULL); }
//    | ST_CLASS T_SYMBOL ST_OPEN_BRACE methods ST_CLOSE_BRACE
//        { $$ = new_class($2, NULL, $4); }
// ;
static struct TopLevelDecl *parse_class(struct Parser *parser)
{
    struct LinkedListNode *old_head = parser->head;
    if (match(parser, T_SYMBOL)) {
        Symbol class_name = nth_token(old_head, 1)->symbol;
        Definitions *defs = parse_class_defs(parser);
        if (!defs) {
            return NULL;
        }
        return new_class(class_name, defs, NULL);
    }
    error_parser("could not parse class");
    return NULL;
}

static struct TopLevelDecl *parse_tld(struct Parser *parser)
{
    struct TopLevelDecl *tld = NULL;
    if (match(parser, ST_FUNCTION)) {
        tld = parse_fundef(parser);
    } else if (match(parser, ST_CLASS)) {
        tld = parse_class(parser);
    } else {
        error_parser("expected function or class.");
    }
    return tld;
}

TopLevelDecls *parse_tlds(struct Parser *parser)
{
    TopLevelDecls *tlds = linkedlist_new();
    do {
        struct TopLevelDecl *tld = parse_tld(parser);
        if (!tld) {
            return NULL;
        }
        linkedlist_append(tlds, tld);
    } while (parser->head != NULL);
    return tlds;
}

#undef match_multiple
#undef parse_call
#undef parse_binexp
