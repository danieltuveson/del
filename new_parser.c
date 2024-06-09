#include "common.h"
#include "allocator.h"
#include "linkedlist.h"
#include "lexer.h"
#include "readfile.h"
#include "ast.h"
#include "printers.h"


// TODO: Used for debugging purposes, delete later
static struct Lexer *debug_lexer;

struct Parser {
    struct LinkedListNode *head;
    struct Lexer *lexer;
};

typedef struct Value *(*ExprParserFunction)(struct Parser *);
typedef struct Expr *(*ExprFunction)(struct Value *, struct Value *);

struct ExprPair {
    enum TokenType type;
    ExprFunction efunc;
};

// Forard declarations
static inline struct Value *parse_expr(struct Parser *parser);
static struct Value *parse_subexpr(struct Parser *parser);

enum MatchType {
    MT_EXPR = INT_MIN,
    MT_PROGRAM,
    MT_TLDS,
    MT_METHODS,
    MT_TLD,
    MT_CLS,
    MT_FUNDEF,
    MT_FUNDEF_ARGS,
    MT_METHOD,
    MT_STATEMENTS,
    MT_BLOCK,
    MT_STATEMENT,
    MT_LINE,
    MT_TYPE,
    MT_CLASS_DEFINITIONS,
    MT_SYMBOLS,
    MT_DEFINITIONS,
    MT_DEFINITION,
    MT_ARGS,
    MT_ACCESSORS,
    MT_ACCESSOR
};

// A Match can either be a MatchType or a TokenType.
// NOTE: if the values of MatchType and TokenType overlap, that will result in compiler bugs.
//       To keep things simple, TokenTypes are undeclared, and thus always positive or 0, and
//       MatchTypes should always be declared as negative
typedef int Match;

static bool parser_match(struct Parser *parser, Match *matches, size_t matches_length)
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
    parser->head = head->next;
    return true;
}

#define match_multiple(parser, matches)\
        parser_match(parser, matches, sizeof(matches) / sizeof(*matches))

static inline bool match(struct Parser *parser, Match match)
{
    return parser_match(parser, &match, 1);
}

// Get nth node
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
    { ST_STAR,  bin_star }, { ST_SLASH, bin_slash },
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

// Technically redundant, since it just calls orexpr. But simplifies things if we ever want to
// create a lower precedence operator than "or"
static inline struct Value *parse_expr(struct Parser *parser)
{
    return parse_orexpr(parser);
}

#undef parse_binexp

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
        printf("Error: unexpected end of argument list\n");
        return NULL;
    }
    return vals;
}

static struct Value *parse_call(struct Parser *parser, Symbol symbol, struct Value *func(Symbol, Values *))
{
    Values *vals = NULL;
    if (match(parser, ST_CLOSE_PAREN)) {
        return func(symbol, NULL);
    } else if ((vals = parse_args(parser))) {
        return func(symbol, vals);
    } else {
        return NULL;
    }
}

Match m_new_obj = { ST_NEW, T_SYMBOL, ST_OPEN_PAREN };
static struct Value *parse_subexpr(struct Parser *parser)
{
    if (parser->head == NULL) {
        printf("Error: unexpected end of expression\n");
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
            return parse_call(parser, symbol, new_vfuncall);
        } else {
            return new_symbol(symbol);
        }
    } else if (match(parser, ST_NEW) && match(parser, T_SYMBOL) && match(parser, ST_OPEN_PAREN)) {
        return parse_call(parser, nth_token(old_head, 2)->symbol, new_constructor);
    }
    printf("Error: unexpected token in expression\n");
    return NULL;
}

#undef match_multiple

// static struct Statement *parse_statement(


int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        goto FAIL;
    }

    printf("........ READING FILE : %s ........\n", argv[1]);
    // TODO: Figure out why this returns error if input is 1 character
    char *input = NULL;
    long input_length = readfile(&input, argv[1]);
    if (input_length == 0) {
        printf("Error: could not read contents of empty file\n");
        goto FAIL;
    }
    printf("%s\n", input);
    print_memory_usage();

    printf("........ TOKENIZING INPUT ........\n");
    struct Lexer lexer;
    lexer_init(&lexer, input, input_length);
    if (!tokenize(&lexer)) {
        print_error(&(lexer.error));
        goto FAIL;
    }
    print_lexer(&lexer);
    print_memory_usage();

    // TODO: delete this, it's only used for debugging
    debug_lexer = &lexer;

    printf("........ PARSING AST FROM TOKENS ........\n");
    struct Parser parser = { lexer.tokens->head, &lexer };
    struct Value *value = parse_expr(&parser);
    if (value) {
        print_value(value);
        printf("\n");
    } else {
        printf("no value. sad\n");
        goto FAIL;
    }
    print_memory_usage();

    ret = EXIT_SUCCESS;
FAIL:
    allocator_freeall();
    return ret;
}
