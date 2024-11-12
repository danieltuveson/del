#include "common.h"
#include "linkedlist.h"
#include "lexer.h"
#include "error.h"
#include "ast.h"
#include "printers.h"
#include "parser.h"

// TODO: improve quality of error messages

typedef struct Value *(*ExprParserFunction)(struct Globals *);
typedef struct Expr *(*ExprFunction)(struct Globals *, struct Value *, struct Value *);

struct ExprPair {
    enum TokenType type;
    ExprFunction efunc;
};

// Forward declarations
static struct Value *parse_subexpr(struct Globals *globals);
static struct Statement *parse_statement(struct Globals *globals);

static inline bool parser_peek(struct Globals *globals, enum TokenType match)
{
    if (globals->parser == NULL) {
        return false;
    }
    struct Token *token = globals->parser->value;
    return token->type == match;
}

// Parses one or more tokens on success, does not consume tokens on failure
static bool parser_match(struct Globals *globals, enum TokenType *matches, size_t matches_length)
{
    if (globals->parser == NULL) {
        return false;
    }
    struct LinkedListNode *head = globals->parser;
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
    globals->parser = head->next;
    return true;
}

#define match_multiple(globals, matches)\
        parser_match(globals, matches, sizeof(matches) / sizeof(*matches))

static inline bool match(struct Globals *globals, enum TokenType match)
{
    return parser_match(globals, &match, 1);
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
static struct Value *parse_binexp_help(struct Globals *globals, ExprParserFunction epfunc,
                                       struct ExprPair *epairs, size_t epairs_length)
{
    struct Value *left = epfunc(globals);
    if (left == NULL) {
        return NULL;
    }
    struct Value *right = NULL;
    while (globals->parser != NULL) {
        for (size_t i = 0; i < epairs_length; i++) {
            enum TokenType type = epairs[i].type;
            ExprFunction efunc = epairs[i].efunc;
            if (match(globals, type)) {
                right = epfunc(globals);
                if (right == NULL) {
                    return NULL;
                }
                left = new_expr(globals, efunc(globals, left, right));
                goto continue_while;
            }
        }
        break;
        continue_while:
        continue;
    }
    return left;
}

#define parse_binexp(globals, epfunc, epairs) \
    parse_binexp_help(globals, epfunc, epairs, sizeof(epairs) / sizeof(*epairs))

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

// static struct ExprPair highest_expr_pairs[] = {
//     { ST_DOT, bin_dot },
//     // { ST_INDEX, bin_index },
//     // { ST_INC, bin_inc },
//     // { ST_DEC, bin_dec },
// };

static inline struct Value *parse_multexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_subexpr, multexpr_pairs);
}

static inline struct Value *parse_addexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_multexpr, addexpr_pairs);
}

static inline struct Value *parse_compexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_addexpr, compexpr_pairs);
}

static inline struct Value *parse_eqexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_compexpr, eqexpr_pairs);
}

static inline struct Value *parse_andexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_eqexpr, andexpr_pairs);
}

static inline struct Value *parse_orexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_andexpr, orexpr_pairs);
}

struct Value *parse_expr(struct Globals *globals)
{
    return parse_orexpr(globals);
}

static Values *parse_args(struct Globals *globals)
{
    struct Value *val = NULL;
    Values *vals = linkedlist_new(globals->allocator);
    do {
        if ((val = parse_expr(globals))) {
            linkedlist_append(vals, val);
        } else {
            return NULL;
        }
    } while (match(globals, ST_COMMA));
    if (!match(globals, ST_CLOSE_PAREN)) {
        error_parser(globals, "Unexpected end of argument list");
        return NULL;
    }
    return vals;
}

#define parse_call(globals, rettype, symbol, func, builtin) \
    do {                                                \
    Values *vals = NULL;                                \
    if (match(globals, ST_CLOSE_PAREN)) {               \
        rettype = func(globals, symbol, NULL, builtin); \
    } else if ((vals = parse_args(globals))) {          \
        rettype = func(globals, symbol, vals, builtin); \
    } else {                                            \
        rettype = NULL;                                 \
    }                                                   \
    } while (false)

static struct Value *parse_vfuncall(struct Globals *globals, Symbol symbol,
    struct Value *func(struct Globals *, Symbol, Values *, bool))
{
    struct Value *val;
    bool builtin = is_builtin(symbol);
    parse_call(globals, val, symbol, func, builtin);
    return val;
}

static struct Value *parse_get(struct Globals *globals, Symbol symbol)
{
    struct LinkedList *lvalues = linkedlist_new(globals->allocator);
    while (true) {
        if (match(globals, ST_DOT)) {
            struct LinkedListNode *old_head = globals->parser;
            if (!match(globals, T_SYMBOL)) {
                error_parser(globals, "Expected property or method name");
                return NULL;
            }
            Symbol prop = nth_token(old_head, 1)->symbol;
            linkedlist_append(lvalues, new_property(globals, prop));
        } else if (match(globals, ST_OPEN_BRACKET)) {
            struct Value *expr = parse_expr(globals);
            if (expr == NULL) {
                return NULL;
            } else if (!match(globals, ST_CLOSE_BRACKET)) {
                error_parser(globals, "Expected closing bracket in accessor");
                return NULL;
            }
            linkedlist_append(lvalues, new_index(globals, expr));
        } else {
            break;
        }
    }
    return new_get(globals, symbol, lvalues);
}

// TODO: Add options for method call
static struct Value *parse_subexpr(struct Globals *globals)
{
    if (globals->parser == NULL) {
        error_parser(globals, "Unexpected end of expression");
        return NULL;
    }
    struct Value *val = NULL;
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, T_INT)) {
        return new_integer(globals, nth_token(old_head, 1)->integer);
    } else if (match(globals, T_FLOAT)) {
        // TODO: parse float
        assert("TODO: parse float" && false);
    } else if (match(globals, T_STRING)) {
        return new_string(globals, nth_token(old_head, 1)->string);
    } else if (match(globals, ST_TRUE)) {
        return new_boolean(globals, 1);
    } else if (match(globals, ST_FALSE)) {
        return new_boolean(globals, 0);
    } else if (match(globals, ST_NULL)) {
        return new_null(globals);
    } else if (match(globals, ST_PLUS) && (val = parse_subexpr(globals))) {
        return new_expr(globals, unary_plus(globals, val));
    } else if (match(globals, ST_MINUS) && (val = parse_subexpr(globals))) {
        return new_expr(globals, unary_minus(globals, val));
    } else if (match(globals, ST_OPEN_PAREN) && (val = parse_expr(globals))
            && match(globals, ST_CLOSE_PAREN)) {
        return val;
    } else if (match(globals, T_SYMBOL)) {
        Symbol symbol = nth_token(old_head, 1)->symbol;
        if (match(globals, ST_OPEN_PAREN)) {
            return parse_vfuncall(globals, symbol, new_vfuncall);
        // } else if (parser_peek(parser, ST_DOT) || parser_peek(parser, ST_OPEN_BRACKET)) {
        } else { // if (parser_peek(parser, ST_DOT) || parser_peek(parser, ST_OPEN_BRACKET)) {
            return parse_get(globals, symbol);
        }
        // } else {
        //     return new_symbol(symbol);
        // }
    } else if (match(globals, ST_NEW) && match(globals, T_SYMBOL) && match(globals, ST_OPEN_PAREN)) {
        return parse_vfuncall(globals, nth_token(old_head, 2)->symbol, new_constructor);
    }
    error_parser(globals, "Unexpected token in expression");
    return NULL;
}

static struct Statement *parse_sfuncall(struct Globals *globals, Symbol symbol,
    struct Statement *func(struct Globals *, Symbol, Values *, bool))
{
    struct Statement *stmt;
    bool builtin = is_builtin(symbol);
    parse_call(globals, stmt, symbol, func, builtin);
    return stmt;
}

static struct Statement *parse_set(struct Globals *globals, Symbol symbol, LValues *lvalues,
    bool is_define)
{
    struct Value *expr;
    if ((expr = parse_expr(globals))) {
        return new_set(globals, symbol, expr, lvalues, is_define);
    }
    return NULL;
}

static Definitions *parse_symbols(struct Globals *globals)
{
    Definitions *defs = linkedlist_new(globals->allocator);
    do {
        struct LinkedListNode *old_head = globals->parser;
        if (match(globals, T_SYMBOL)) {
            Symbol symbol = nth_token(old_head, 1)->symbol;
            linkedlist_append(defs, new_define(globals, symbol, TYPE_UNDEFINED));
        } else {
            error_parser(globals, "Unexpected token in definition");
            return NULL;
        }
    } while (match(globals, ST_COMMA));
    return defs;
}

// accessors: accessor accessors { $$ = append($2, $1); }
//          | accessor ST_EQ { $$ = new_list($1); }
// 
// accessor: ST_DOT T_SYMBOL { $$ = new_property($2); }
//         | ST_OPEN_BRACKET T_INT ST_CLOSE_BRACKET { $$ = new_index(new_integer($2)); }
// TODO: Add options for method call
static struct Statement *parse_lhs(struct Globals *globals, Symbol symbol)
{
    struct LinkedList *lvalues = linkedlist_new(globals->allocator);
    while (true) {
        if (match(globals, ST_EQ)) {
            return parse_set(globals, symbol, lvalues, false);
        } else if (match(globals, ST_OPEN_PAREN)) {
            // assert("TODO: update parse_sfuncall to accept accessors" && false);
            // return NULL;
            return parse_sfuncall(globals, symbol, new_sfuncall);
        }
        if (match(globals, ST_DOT)) {
            struct LinkedListNode *old_head = globals->parser;
            if (!match(globals, T_SYMBOL)) {
                error_parser(globals, "Expected property or method name");
                return NULL;
            }
            Symbol prop = nth_token(old_head, 1)->symbol;
            linkedlist_append(lvalues, new_property(globals, prop));
        } else if (match(globals, ST_OPEN_BRACKET)) {
            struct Value *expr = parse_expr(globals);
            if (expr == NULL) {
                return NULL;
            } else if (!match(globals, ST_CLOSE_BRACKET)) {
                error_parser(globals, "Expected closing bracket in accessor");
                return NULL;
            }
            linkedlist_append(lvalues, new_index(globals, expr));
        } else {
            break;
        }
    }
    error_parser(globals, "Unexpected value in accessor");
    return NULL;
}

static struct Statement *parse_line(struct Globals *globals)
{
    if (globals->parser == NULL) {
        error_parser(globals, "Unexpected end of input. Expected start of statement");
        return NULL;
    }
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, T_SYMBOL)) {
        Symbol symbol = nth_token(old_head, 1)->symbol;
        return parse_lhs(globals, symbol);
    } else if (match(globals, ST_LET)) {
        Definitions *defs = NULL;
        enum TokenType matches[] = { T_SYMBOL, ST_EQ };
        if (match_multiple(globals, matches)) {
            Symbol symbol = nth_token(old_head, 2)->symbol;
            return parse_set(globals, symbol, linkedlist_new(globals->allocator), true);
        } else if ((defs = parse_symbols(globals))) {
            return new_let(globals, defs);
        }
        return NULL;
    } else if (match(globals, ST_RETURN)) {
        struct Value *expr = NULL;
        if (parser_peek(globals, ST_SEMICOLON)) {
            return new_return(globals, NULL);
        } else if ((expr = parse_expr(globals))) {
            return new_return(globals, expr);
        }
        return NULL;
    }
    error_parser(globals, "Unexpected token in statement");
    return NULL;
}

static Statements *parse_block(struct Globals *globals, char *type)
{
    if (!match(globals, ST_OPEN_BRACE)) {
        error_parser(globals, "%s missing opening brace", type);
        return NULL;
    }
    struct Statement *stmt = NULL;
    Statements *stmts = linkedlist_new(globals->allocator);
    while (!match(globals, ST_CLOSE_BRACE)) {
        stmt = parse_statement(globals);
        if (!stmt) {
            return NULL;
        }
        linkedlist_append(stmts, stmt);
    }
    return stmts;
}

static struct Statement *parse_if_branch(struct Globals *globals, char *type)
{
    struct Value *condition = parse_expr(globals);
    if (!condition) {
        return NULL;
    }
    Statements *if_stmts = parse_block(globals, type);
    if (!if_stmts) {
        return NULL;
    }
    return new_if(globals, condition, if_stmts);
}

static struct Statement *parse_if(struct Globals *globals)
{
    struct Statement *first_if_stmt = parse_if_branch(globals, "if block");
    if (!first_if_stmt) {
        return NULL;
    }
    struct Statement *stmt = first_if_stmt;
    enum TokenType elseif[] = { ST_ELSE, ST_IF };
    while (match_multiple(globals, elseif)) {
        struct Statement *elseif_stmt = parse_if_branch(globals, "else if block");
        if (!elseif_stmt) {
            return NULL;
        }
        stmt = add_elseif(globals, stmt->if_stmt, elseif_stmt);
    }
    if (match(globals, ST_ELSE)) {
        Statements *else_stmts = parse_block(globals, "else block");
        if (!else_stmts) {
            return NULL;
        }
        add_else(stmt->if_stmt, else_stmts);
    }
    return first_if_stmt;
}

static struct Statement *parse_statement(struct Globals *globals)
{
    struct Value *expr = NULL;
    struct Statement *stmt = NULL;
    Statements *stmts = NULL;
    if (match(globals, ST_IF)) {
        if ((stmt = parse_if(globals))) {
            return stmt;
        }
        return NULL;
    } else if (match(globals, ST_FOR)) {
        struct Statement *init, *increment;
        // Parens are allowed to surround for loop arguments but are not required
        bool redundant_paren = match(globals, ST_OPEN_PAREN);
        if ((init = parse_line(globals)) && match(globals, ST_SEMICOLON)
                && (expr = parse_expr(globals)) && match(globals, ST_SEMICOLON) 
                && (increment = parse_line(globals))) {
            if (redundant_paren != match(globals, ST_CLOSE_PAREN)) {
                error_parser(globals, "mismatched parenthesis");
                return NULL;
            } else if ((stmts = parse_block(globals, "for block"))) {
                return new_for(globals, init, expr, increment, stmts);
            }
        }
        return NULL;
    } else if (match(globals, ST_WHILE)) {
        if ((expr = parse_expr(globals))) {
            if ((stmts = parse_block(globals, "while block"))) {
                return new_while(globals, expr, stmts);
            }
        }
        return NULL;
    } else if ((stmt = parse_line(globals))) {
        if (match(globals, ST_SEMICOLON)) {
            return stmt;
        }
        error_parser(globals, "Unexpected end of statement");
    } 
    return NULL;
}

// type: ST_INT { $$ = TYPE_INT; }
//     | ST_FLOAT { $$ = TYPE_FLOAT; }
//     | ST_BOOL { $$ = TYPE_BOOL; }
//     | ST_STRING { $$ = TYPE_STRING; }
//     | T_SYMBOL { $$ = $1; }
// ;
static Type parse_type(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, ST_INT)) {
        return TYPE_INT;
    } else if (match(globals, ST_FLOAT)) {
        return TYPE_FLOAT;
    } else if (match(globals, ST_BOOL)) {
        return TYPE_BOOL;
    } else if (match(globals, ST_STRING)) {
        return TYPE_STRING;
    } else if (match(globals, T_SYMBOL)) {
        return nth_token(old_head, 1)->symbol;
    }
    error_parser(globals, "invalid type");
    return TYPE_UNDEFINED;
}

// definitions: definition { $$ = new_list($1); }
//            | definition ST_COMMA definitions { $$ = append($3, $1); }
// ;
// 
// definition: T_SYMBOL ST_COLON type { $$ = new_define($1, $3); };
static struct Definition *parse_definition(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    Type type = TYPE_UNDEFINED;
    if (!match(globals, T_SYMBOL)) {
        error_parser(globals, "expected variable name");
        return NULL;
    } else if (match(globals, ST_COMMA) || match(globals, ST_CLOSE_PAREN)) {
        error_parser(globals, "function argument is missing a type");
        return NULL;
    } else if (!(match(globals, ST_COLON))) {
        error_parser(globals, "expected colon");
        return NULL;
    } else if ((type = parse_type(globals)) == TYPE_UNDEFINED) {
        error_parser(globals, "invalid type");
        return NULL;
    }
    return new_define(globals, nth_token(old_head, 1)->symbol, type);
}

// fundef_args: ST_OPEN_PAREN definitions ST_CLOSE_PAREN { $$ = $2; }
//            | ST_OPEN_PAREN ST_CLOSE_PAREN { $$ = NULL; }
// ;
static Definitions *parse_fundef_args(struct Globals *globals)
{
    if (!match(globals, ST_OPEN_PAREN)) {
        error_parser(globals, "expected list of function arguments");
        return NULL;
    }
    Definitions *definitions = linkedlist_new(globals->allocator);
    if (match(globals, ST_CLOSE_PAREN)) {
        return definitions;
    }
    do {
        struct Definition *definition = parse_definition(globals);
        if (!definition) {
            return NULL;
        }
        linkedlist_append(definitions, definition);
    } while (match(globals, ST_COMMA));
    if (!match(globals, ST_CLOSE_PAREN)) {
        error_parser(globals, "Unexpected end of argument list");
        return NULL;
    }
    return definitions;
}

// fundef: ST_FUNCTION T_SYMBOL fundef_args ST_COLON type ST_OPEN_BRACE statements ST_CLOSE_BRACE
//       { $$ = new_tld_fundef($2, $5, $3, $7); }
//       | ST_FUNCTION T_SYMBOL fundef_args ST_OPEN_BRACE statements ST_CLOSE_BRACE
// { $$ = new_tld_fundef($2, TYPE_UNDEFINED, $3, $5); }
// ;
static struct TopLevelDecl *parse_fundef(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, T_SYMBOL)) {
        Symbol funname = nth_token(old_head, 1)->symbol;
        if (is_builtin(funname)) {
            error_parser(globals, "cannot use builtin function name as declaration");
            return NULL;
        }
        Definitions *defs = parse_fundef_args(globals);
        Type rettype = TYPE_UNDEFINED;
        if (!defs) {
            return NULL;
        } else if (match(globals, ST_COLON)) {
            rettype = parse_type(globals);
            if (rettype == TYPE_UNDEFINED) {
                return NULL;
            }
        }
        Statements *stmts = parse_block(globals, lookup_symbol(globals, funname));
        if (!stmts) {
            return NULL;
        }
        return new_tld_fundef(globals, funname, rettype, defs, stmts);
    }
    error_parser(globals, "could not parse function");
    return NULL;
}

static struct Definition *parse_class_definition(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    Type type = TYPE_UNDEFINED;
    if (!match(globals, T_SYMBOL)) {
        error_parser(globals, "expected property name");
        return NULL;
    } else if (match(globals, ST_SEMICOLON)) {
        error_parser(globals, "class property is missing a type");
        return NULL;
    } else if (!(match(globals, ST_COLON))) {
        error_parser(globals, "expected colon");
        return NULL;
    } else if ((type = parse_type(globals)) == TYPE_UNDEFINED) {
        error_parser(globals, "invalid type");
        return NULL;
    }
    return new_define(globals, nth_token(old_head, 1)->symbol, type);
}

// class_definitions: definition ST_SEMICOLON { $$ = new_list($1); }
//                  | definition ST_SEMICOLON class_definitions { $$ = append($3, $1); }
// ;
static Definitions *parse_class_defs(struct Globals *globals)
{
    if (!match(globals, ST_OPEN_BRACE)) {
        error_parser(globals, "expected list of class properties");
        return NULL;
    }
    Definitions *definitions = linkedlist_new(globals->allocator);
    do {
        if (match(globals, ST_CLOSE_BRACE)) {
            return definitions;
        }
        struct Definition *definition = parse_class_definition(globals);
        if (!definition) {
            return NULL;
        }
        linkedlist_append(definitions, definition);
    } while (match(globals, ST_SEMICOLON));
    if (!match(globals, ST_CLOSE_BRACE)) {
        error_parser(globals, "Unexpected end of property list");
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
static struct TopLevelDecl *parse_class(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, T_SYMBOL)) {
        Symbol class_name = nth_token(old_head, 1)->symbol;
        Definitions *defs = parse_class_defs(globals);
        if (!defs) {
            return NULL;
        }
        return new_class(globals, class_name, defs, NULL);
    }
    error_parser(globals, "could not parse class");
    return NULL;
}

static struct TopLevelDecl *parse_tld(struct Globals *globals)
{
    struct TopLevelDecl *tld = NULL;
    if (match(globals, ST_FUNCTION)) {
        tld = parse_fundef(globals);
    } else if (match(globals, ST_CLASS)) {
        tld = parse_class(globals);
    } else {
        error_parser(globals, "expected function or class.");
    }
    return tld;
}

TopLevelDecls *parse_tlds(struct Globals *globals)
{
    TopLevelDecls *tlds = linkedlist_new(globals->allocator);
    do {
        struct TopLevelDecl *tld = parse_tld(globals);
        if (!tld) {
            return NULL;
        }
        linkedlist_append(tlds, tld);
    } while (globals->parser != NULL);
    return tlds;
}

#undef match_multiple
#undef parse_call
#undef parse_binexp
