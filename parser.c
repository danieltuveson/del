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
static struct Value *parse_unaryexpr(struct Globals *globals);
static struct Statement *parse_statement(struct Globals *globals);
static Type parse_type(struct Globals *globals);
static Definitions *parse_typed_args(struct Globals *globals);

// Common error messages
const char *expected_semicolon = "Expected semicolon";

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
    { ST_PLUS, bin_plus }, { ST_MINUS, bin_minus },
};

static struct ExprPair multexpr_pairs[] = {
    { ST_STAR, bin_star }, { ST_SLASH, bin_slash }, { ST_PERCENT, bin_percent },
};

static inline struct Value *parse_multexpr(struct Globals *globals)
{
    return parse_binexp(globals, parse_unaryexpr, multexpr_pairs);
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

static struct Value *parse_vfuncall(struct Globals *globals, struct Accessor *access)
{
    bool builtin = is_builtin(access->definition->name);
    if (match(globals, ST_CLOSE_PAREN)) {
        return new_vfuncall(globals, access, NULL, builtin);
    }
    Values *vals = parse_args(globals);
    if (vals != NULL) {
        return new_vfuncall(globals, access, vals, builtin);
    }
    return NULL;
}

static Types *parse_type_args(struct Globals *globals)
{
    if (match(globals, ST_GREATER)) {
        error_parser(globals, "Cannot have empty type list");
        return NULL;
    }
    Types *types = linkedlist_new(globals->allocator);
    do {
        Type type = parse_type(globals);
        if (type == TYPE_UNDEFINED) {
            return NULL;
        } else if (is_array(type)) {
            error_parser(globals, "Array may not contain other arrays");
            return NULL;
        }
        Type *type_ptr = allocator_malloc(globals->allocator, sizeof(*type_ptr));
        *type_ptr = type;
        linkedlist_append(types, type_ptr);
    } while (match(globals, ST_COMMA));
    if (!match(globals, ST_GREATER)) {
        error_parser(globals, "Unexpected end of type arguments");
        return NULL;
    }
    return types;
}

static struct Value *parse_constructor(struct Globals *globals, Symbol symbol)
{
    bool builtin = is_builtin(symbol);
    Types *types = NULL;
    Values *vals = NULL;
    if (match(globals, ST_LESS)) {
        types = parse_type_args(globals);
        if (types == NULL) {
            return NULL;
        }
    }
    if (!match(globals, ST_OPEN_PAREN)) {
        error_parser(globals, "Expected list of arguments");
        return NULL;
    }
    struct Accessor *a = new_accessor(globals, symbol, linkedlist_new(globals->allocator));
    if (match(globals, ST_CLOSE_PAREN)) {
        return new_constructor(globals, a, types, NULL, builtin);
    } else if ((vals = parse_args(globals))) {
        return new_constructor(globals, a, types, vals, builtin);
    }
    return NULL;
}

static struct Value *parse_property_access(struct Globals *globals, struct Value *val)
{
    while (true) {
        struct LinkedListNode *old_head = globals->parser;
        if (match(globals, ST_OPEN_PAREN)) {
            if (val->vtype != VTYPE_GET_LOCAL) {
                printf("Error: accessor to function or classes not implemented\n");
                assert(false);
            }
            Symbol funname = val->get_local->name;
            struct Accessor *a = new_accessor(globals, funname, linkedlist_new(globals->allocator));
            val = parse_vfuncall(globals, a);
        } else if (match(globals, ST_DOT)) {
            if (!match(globals, T_SYMBOL)) {
                error_parser(globals, "Expected property or method name");
                return NULL;
            }
            Symbol property = nth_token(old_head, 2)->symbol;
            val = new_get_property(globals, val, property);
        } else if (match(globals, ST_OPEN_BRACKET)) {
            struct Value *index = parse_expr(globals);
            if (index == NULL) {
                return NULL;
            } else if (!match(globals, ST_CLOSE_BRACKET)) {
                error_parser(globals, "Expected closing bracked");
                return NULL;
            }
            val = new_get_indexed(globals, val, index);
        } else if (match(globals, ST_CAST)) {
            Type type = parse_type(globals);
            if (TYPE_UNDEFINED) {
                return NULL;
            }
            return new_cast(globals, val, type);
        } else {
            break;
        }
    }
    return val;
}

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
        return new_floating(globals, nth_token(old_head, 1)->floating);
    } else if (match(globals, T_STRING)) {
        return new_string(globals, nth_token(old_head, 1)->string);
    } else if (match(globals, T_BYTE)) {
        return new_byte(globals, nth_token(old_head, 1)->byte);
    } else if (match(globals, ST_TRUE)) {
        return new_boolean(globals, 1);
    } else if (match(globals, ST_FALSE)) {
        return new_boolean(globals, 0);
    } else if (match(globals, ST_NULL)) {
        return new_null(globals);
    } else if (match(globals, ST_OPEN_PAREN) && (val = parse_expr(globals))
            && match(globals, ST_CLOSE_PAREN)) {
        return parse_property_access(globals, val);
    } else if (match(globals, T_SYMBOL)) {
        Symbol variable = nth_token(old_head, 1)->symbol;
        val = new_get_local(globals, variable);
        return parse_property_access(globals, val);
    } else if (match(globals, ST_NEW) && match(globals, T_SYMBOL)) {
        return parse_constructor(globals, nth_token(old_head, 2)->symbol);
    }
    error_parser(globals, "Unexpected token in expression");
    return NULL;
}

static struct Value *parse_unaryexpr(struct Globals *globals)
{
    struct Value *val = NULL;
    if (match(globals, ST_PLUS) && (val = parse_subexpr(globals))) {
        return new_expr(globals, unary_plus(globals, val));
    } else if (match(globals, ST_MINUS) && (val = parse_subexpr(globals))) {
        return new_expr(globals, unary_minus(globals, val));
    }
    // else if (match(globals, ST_NOT) && (val = parse_subexpr) etc...
    return parse_subexpr(globals);
}

// static Definitions *parse_symbols(struct Globals *globals)
// {
//     Definitions *defs = linkedlist_new(globals->allocator);
//     do {
//         struct LinkedListNode *old_head = globals->parser;
//         if (match(globals, T_SYMBOL)) {
//             Symbol symbol = nth_token(old_head, 1)->symbol;
//             linkedlist_append(defs, new_define(globals, symbol, TYPE_UNDEFINED));
//         } else {
//             error_parser(globals, "Unexpected token in definition");
//             return NULL;
//         }
//     } while (match(globals, ST_COMMA));
//     return defs;
// }

static struct Statement *parse_line(struct Globals *globals)
{
    if (globals->parser == NULL) {
        error_parser(globals, "Unexpected end of input. Expected start of statement");
        return NULL;
    }
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, ST_LET)) {
        Definitions *defs = NULL;
        enum TokenType matches[] = { T_SYMBOL, ST_EQ };
        if (match_multiple(globals, matches)) {
            Symbol symbol = nth_token(old_head, 2)->symbol;
            struct Value *expr;
            if ((expr = parse_expr(globals))) {
                return new_set_local(globals, symbol, expr, true);
            }
            return NULL;
        } else if ((defs = parse_typed_args(globals))) {
            return new_let(globals, defs);
        }
        return NULL;
    } else if (match(globals, ST_BREAK)) {
        return new_break(globals);
    } else if (match(globals, ST_CONTINUE)) {
        return new_continue(globals);
    } else if (match(globals, ST_RETURN)) {
        struct Value *expr = NULL;
        if (parser_peek(globals, ST_SEMICOLON)) {
            return new_return(globals, NULL);
        } else if ((expr = parse_expr(globals))) {
            return new_return(globals, expr);
        }
        return NULL;
    }
    struct Value *val = parse_expr(globals);
    if (!val) {
        return NULL;
    }
    // if (val->vtype != VTYPE_GET_LOCAL && val->vtype !=
    if (match(globals, ST_INC)) {
        if (val->vtype == VTYPE_EXPR) {
            error_parser(globals, "Increment cannot be used as an expression");
            return NULL;
        } else if (val->vtype != VTYPE_GET_LOCAL && val->vtype != VTYPE_GET_PROPERTY
                && val->vtype != VTYPE_INDEX) {
            error_parser(globals, "Invalid use of increment");
            return NULL;
        }
        return new_increment(globals, val);
    } else if (match(globals, ST_DEC)) {
        if (val->vtype == VTYPE_EXPR) {
            error_parser(globals, "Decrement cannot be used as an expression");
            return NULL;
        } else if (val->vtype != VTYPE_GET_LOCAL && val->vtype != VTYPE_GET_PROPERTY
                && val->vtype != VTYPE_INDEX) {
            error_parser(globals, "Invalid use of decrement");
            return NULL;
        }
        return new_decrement(globals, val);
    }
    struct Statement *stmt = NULL;
    struct Value *rhs = NULL;
    if (val->vtype == VTYPE_GET_LOCAL || val->vtype == VTYPE_GET_PROPERTY
            || val->vtype == VTYPE_INDEX) {
        if (!match(globals, ST_EQ)) {
            goto generic_error;
        }
        rhs = parse_expr(globals);
        if (!rhs) {
            return NULL;
        }
    }
    struct GetProperty *get = NULL;
    switch (val->vtype) {
        case VTYPE_GET_LOCAL:
            return new_set_local(globals, val->get_local->name, rhs, false);
        case VTYPE_GET_PROPERTY:
            get = val->get_property;
            return new_set_property(globals, get->accessor, get->property, rhs);
        case VTYPE_INDEX:
            get = val->get_property;
            return new_set_indexed(globals, get->accessor, get->index, rhs);
        case VTYPE_FUNCALL:
            stmt = new_stmt(globals, STMT_FUNCALL);
            stmt->funcall = val->funcall;
            return stmt;
        case VTYPE_BUILTIN_FUNCALL:
            stmt = new_stmt(globals, STMT_BUILTIN_FUNCALL);
            stmt->funcall = val->funcall;
            return stmt;
        default:
            break;
    }
generic_error:
    error_parser(globals, "Expected a statement but got an expression");
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

static struct Statement *parse_for(struct Globals *globals)
{
    // Parens are allowed to surround for loop arguments but are not required
    bool redundant_paren = match(globals, ST_OPEN_PAREN);
    struct Statement *init = parse_line(globals);
    if (init == NULL) {
        return NULL;
    } else if (!match(globals, ST_SEMICOLON)) {
        error_parser(globals, expected_semicolon);
        return NULL;
    }
    struct Value *expr = parse_expr(globals);
    if (expr == NULL) {
        return NULL;
    } else if (!match(globals, ST_SEMICOLON)) {
        error_parser(globals, expected_semicolon);
        return NULL;
    }
    struct Statement *increment = parse_line(globals);
    if (increment == NULL) {
        return NULL;
    } else if (redundant_paren != match(globals, ST_CLOSE_PAREN)) {
        error_parser(globals, "Mismatched parenthesis");
        return NULL;
    }
    Statements *stmts = parse_block(globals, "for block");
    if (stmts == NULL) {
        return NULL;
    }
    return new_for(globals, init, expr, increment, stmts);
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
        return parse_for(globals);
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
    } else if (match(globals, ST_BYTE)) {
        return TYPE_BYTE;
    } else if (match(globals, T_SYMBOL)) {
        Symbol symbol = nth_token(old_head, 1)->symbol;
        if (symbol != BUILTIN_ARRAY) {
            return symbol;
        } else if (!match(globals, ST_LESS)) {
            error_parser(globals, "Array declaration is missing type");
            return TYPE_UNDEFINED;
        }
        Types *types = parse_type_args(globals);
        if (types == NULL) {
            return TYPE_UNDEFINED;
        } else if (types->length != 1) {
            // TODO: Do proper generics, eventually
            error_parser(globals, "Array declaration has too many types");
            return TYPE_UNDEFINED;
        }
        Type *type_ptr = types->head->value;
        if (is_array(*type_ptr)) {
            error_parser(globals, "Array may not contain other arrays");
            return TYPE_UNDEFINED;
        }
        return array_of(*type_ptr);
    }
    error_parser(globals, "Invalid type");
    return TYPE_UNDEFINED;
}

static struct Definition *parse_definition(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    Type type = TYPE_UNDEFINED;
    if (!match(globals, T_SYMBOL)) {
        error_parser(globals, "Expected variable name");
        return NULL;
    } else if (parser_peek(globals, ST_COMMA) || parser_peek(globals, ST_CLOSE_PAREN)
            || parser_peek(globals, ST_SEMICOLON)) {
        error_parser(globals, "Variable is missing a type");
        return NULL;
    } else if (!(parser_peek(globals, ST_COLON))) {
        error_parser(globals, "Expected colon");
        return NULL;
    }
    match(globals, ST_COLON);
    if ((type = parse_type(globals)) == TYPE_UNDEFINED) {
        return NULL;
    }
    return new_define(globals, nth_token(old_head, 1)->symbol, type);
}

static Definitions *parse_typed_args(struct Globals *globals)
{
    Definitions *definitions = linkedlist_new(globals->allocator);
    do {
        struct Definition *definition = parse_definition(globals);
        if (!definition) {
            return NULL;
        }
        linkedlist_append(definitions, definition);
    } while (match(globals, ST_COMMA));
    return definitions;
}

static Definitions *parse_fundef_args(struct Globals *globals)
{
    if (!match(globals, ST_OPEN_PAREN)) {
        error_parser(globals, "Expected list of arguments");
        return NULL;
    } else if (match(globals, ST_CLOSE_PAREN)) {
        return linkedlist_new(globals->allocator);
    }
    Definitions *definitions = parse_typed_args(globals);
    if (!match(globals, ST_CLOSE_PAREN)) {
        error_parser(globals, "Unexpected end of argument list");
        return NULL;
    }
    return definitions;
}

static struct TopLevelDecl *parse_fundef(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    if (match(globals, T_SYMBOL)) {
        Symbol funname = nth_token(old_head, 1)->symbol;
        if (is_builtin(funname)) {
            error_parser(globals, "Cannot use builtin function name as declaration");
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
    error_parser(globals, "Could not parse function");
    return NULL;
}

static bool parse_class_definition(struct Globals *globals, Definitions *definitions,
        TopLevelDecls *methods)
{
    struct LinkedListNode *old_head = globals->parser;
    Type type = TYPE_UNDEFINED;
    char *err_msg = "expected property or method name";
    if (parser_peek(globals, ST_FUNCTION) || parser_peek(globals, ST_GET)
                || parser_peek(globals, ST_SET) || parser_peek(globals, ST_CONSTRUCTOR)) {
        if (!match(globals, ST_FUNCTION)) {
            printf("non-standard methods not yet implemented\n");
            assert(false);
        }
        struct TopLevelDecl *tld = parse_fundef(globals);
        if (tld == NULL) {
            return false;
        }
        linkedlist_append(methods, tld);
        return true;
    } else if (match(globals, T_SYMBOL)) {
        Symbol symbol = nth_token(old_head, 1)->symbol;
        if (!match(globals, ST_COLON)) {
            error_parser(globals, "Expected type declaration");
            return false;
        } else if ((type = parse_type(globals)) == TYPE_UNDEFINED) {
            return false;
        } else if (!match(globals, ST_SEMICOLON)) {
            error_parser(globals, expected_semicolon);
            return false;
        }
        struct Definition *def = new_define(globals, symbol, type);
        linkedlist_append(definitions, def);
        return true;
    }
    error_parser(globals, err_msg);
    return false;
}

static struct TopLevelDecl *parse_class(struct Globals *globals)
{
    struct LinkedListNode *old_head = globals->parser;
    if (!match(globals, T_SYMBOL)) {
        error_parser(globals, "Expected class name");
        return NULL;
    }
    Symbol class_name = nth_token(old_head, 1)->symbol;
    Definitions *definitions = linkedlist_new(globals->allocator);
    TopLevelDecls *methods = linkedlist_new(globals->allocator);
    struct TopLevelDecl *tld = new_class(globals, class_name, definitions, methods);
    if (!match(globals, ST_OPEN_BRACE)) {
        error_parser(globals, "Expected '{'");
        return NULL;
    }
    if (match(globals, ST_CLOSE_BRACE)) {
        return tld;
    }
    do {
        if (!parse_class_definition(globals, definitions, methods)) {
            return NULL;
        }
    } while (!parser_peek(globals, ST_CLOSE_BRACE));
    assert(match(globals, ST_CLOSE_BRACE));
    return tld;
}

static struct TopLevelDecl *parse_tld(struct Globals *globals)
{
    struct TopLevelDecl *tld = NULL;
    if (match(globals, ST_FUNCTION)) {
        tld = parse_fundef(globals);
    } else if (match(globals, ST_CLASS)) {
        tld = parse_class(globals);
    } else {
        error_parser(globals, "Expected function or class.");
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
#undef parse_binexp

