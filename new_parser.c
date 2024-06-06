#include "common.h"
#include "linkedlist.h"
#include "lexer.h"
#include "readfile.h"
#include "ast.h"
#include "printers.h"


// TODO: Used for debugging purposes, delete later
struct Lexer *debug_lexer;

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

// Forward declarations
static struct Value *parse_addexpr(struct Parser *parser);
static struct Value *parse_multexpr(struct Parser *parser);
static struct Value *parse_subexpr(struct Parser *parser);

// expr: T_INT { $$ = new_integer($1); }
//     | T_SYMBOL { $$ = new_symbol($1); }
//     | T_STRING { $$ = new_string($1); }
//     | ST_TRUE { $$ = new_boolean(1); }
//     | ST_FALSE { $$ = new_boolean(0); }
//     | ST_OPEN_PAREN subexpr ST_CLOSE_PAREN { $$ = $2; }
//     | T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_vfuncall($1, $3); }
//     | T_SYMBOL ST_OPEN_PAREN ST_CLOSE_PAREN { $$ = new_vfuncall($1, NULL); }
//     | ST_NEW T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_constructor($2, $4); }
//     | ST_NEW T_SYMBOL ST_OPEN_PAREN ST_CLOSE_PAREN { $$ = new_constructor($2, NULL); }
//     | subexpr
// ;
// 
// args: expr { $$ = new_list($1); }
//     | expr ST_COMMA args { $$ = append($3, $1); }
// ;
// 
// subexpr: ST_PLUS expr %prec UNARY_PLUS { $$ = new_expr(unary_plus($2)); }
//     | ST_MINUS expr %prec UNARY_MINUS { $$ = new_expr(unary_minus($2)); }
//     | expr ST_PLUS expr { $$ = new_expr(bin_plus($1, $3)); }
//     | expr ST_MINUS expr { $$ = new_expr(bin_minus($1, $3)); }
//     | expr ST_STAR expr { $$ = new_expr(bin_star($1, $3)); }
//     | expr ST_SLASH expr { $$ = new_expr(bin_slash($1, $3)); }
//     | expr ST_OR expr { $$ = new_expr(bin_or($1, $3)); }
//     | expr ST_AND expr { $$ = new_expr(bin_and($1, $3)); }
//     | expr ST_EQEQ expr { $$ = new_expr(bin_eqeq($1, $3)); }
//     | expr ST_NOT_EQ expr { $$ = new_expr(bin_not_eq($1, $3)); }
//     | expr ST_GREATER_EQ expr { $$ = new_expr(bin_greater_eq($1, $3)); }
//     | expr ST_LESS_EQ expr { $$ = new_expr(bin_less_eq($1, $3)); }
//     | expr ST_GREATER expr { $$ = new_expr(bin_greater($1, $3)); }
//     | expr ST_LESS expr { $$ = new_expr(bin_less($1, $3)); }
// ;

enum MatchType {
    MT_EXPR = INT_MIN,
    MT_ANDEXPR,
    MT_EQEXPR,
    MT_COMPEXPR,
    MT_ADDEXPR,
    MT_MULTEXPR,
    MT_SUBEXPR,
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

bool parser_match(struct Parser *parser, Match *matches, size_t matches_length)
{
    struct LinkedListNode *head = parser->head;
    size_t i = 0;
    linkedlist_foreach(lnode, head) {
        if (i >= matches_length) {
            printf("breaking\n");
            break;
        }
        printf("trying to match stuff...\n");
        struct Token *token = lnode->value;
        if (token->type != matches[i]) {
            printf("no match\n");
            return false;
        }
        printf("token type: %d\n", token->type);
        printf("match: %d\n", matches[i]);
        i++;
        head = lnode;
    }
    printf("got a match\n");
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
static struct LinkedListNode *nth_arg(struct LinkedListNode *head, int num)
{
    int i = 1; // starting index at 1 to match flex/bison
    linkedlist_foreach(lnode, head) {
        printf("token: ");
        print_token(debug_lexer, lnode->value);
        if (i == num) {
            return lnode;
        }
        i++;
    }
    return NULL;
}

static inline struct Token *nth_token(struct LinkedListNode *head, int num)
{
    struct LinkedListNode *lnode = nth_arg(head, num);
    return lnode == NULL ? NULL : lnode->value;
}

// int st_plus = { MT_EXPR, ST_PLUS, MT_EXPR };
// static struct Value *parse_subexpr(struct Parser *parser)
// {
//     if      (match(parser, st_plus))      return new_integer(nth_token(head, 1)->integer);
// 
//     printf("Error: could not match expression\n");
//     return NULL;
// }

static struct Value *parse_expr(struct Parser *parser)
{
    return parse_addexpr(parser);
}

// Cursed helper function for parsing expressions.
// I would have had to write basically the same code for every binary expression, but instead I'm
// using function pointers to DRY it up. I know it looks heinous.
static struct Value *parse_binexp_help(struct Parser *parser, ExprParserFunction epfunc,
                                       struct ExprPair *epairs, size_t epairs_length)
{
    struct Value *left = parse_subexpr(parser);
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

struct ExprPair addexpr_pairs[] = {
    { ST_PLUS,  bin_plus  },
    { ST_MINUS, bin_minus }
};

static struct Value *parse_addexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_multexpr, addexpr_pairs);
}

struct ExprPair multexpr_pairs[] = {
    { ST_STAR,  bin_star  },
    { ST_SLASH, bin_slash }
};
static struct Value *parse_multexpr(struct Parser *parser)
{
    return parse_binexp(parser, parse_subexpr, multexpr_pairs);
}

#undef parse_binexp

// static struct Value *parse_multexpr(struct Parser *parser)
// {
//     struct Value *left = parse_subexpr(parser);
//     if (left == NULL) {
//         return NULL;
//     }
//     struct Value *right = NULL;
//     while (parser->head != NULL) {
//         if (match(parser, ST_STAR)) {
//             right = parse_subexpr(parser);
//             if (right == NULL) {
//                 return NULL;
//             }
//             left = new_expr(bin_star(left, right));
//         } else if (match(parser, ST_SLASH)) {
//             right = parse_subexpr(parser);
//             if (right == NULL) {
//                 return NULL;
//             }
//             left = new_expr(bin_slash(left, right));
//         } else {
//             break;
//         }
//     }
//     return left;
// }

// int paren_expr[] = { ST_OPEN_PAREN, T_INT, ST_CLOSE_PAREN }; // MT_SUBEXPR, ST_CLOSE_PAREN };

static struct Value *parse_subexpr(struct Parser *parser)
{
    if (parser->head == NULL) {
        printf("Error: unexpected end of expression\n");
        return NULL;
    }
    struct Value *val = NULL;
    struct LinkedListNode *old_head = parser->head;
    if      (match(parser, T_INT))      return new_integer(nth_token(old_head, 1)->integer);
    else if (match(parser, T_SYMBOL))   return new_symbol(nth_token(old_head, 1)->symbol);
    else if (match(parser, T_STRING))   return new_string(nth_token(old_head, 1)->string);
    else if (match(parser, ST_TRUE))    return new_boolean(1);
    else if (match(parser, ST_FALSE))   return new_boolean(0);
    // example of multi-argument
    // else if (match(parser, paren_expr)) return new_integer(nth_token(head, 2)->integer);

    // ST_OPEN_PAREN subexpr ST_CLOSE_PAREN { $$ = $2; }
    // T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_vfuncall($1, $3); }
    // T_SYMBOL ST_OPEN_PAREN ST_CLOSE_PAREN { $$ = new_vfuncall($1, NULL); }
    // ST_NEW T_SYMBOL ST_OPEN_PAREN args ST_CLOSE_PAREN { $$ = new_constructor($2, $4); }
    // ST_NEW T_SYMBOL ST_OPEN_PAREN ST_CLOSE_PAREN { $$ = new_constructor($2, NULL); }
    // subexpr
    printf("Error: unexpected token in expression\n");
    return NULL;
}

#undef match_multiple


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        return 1;
    }
    printf("........ READING FILE : %s ........\n", argv[1]);
    char *input = NULL;
    int input_length = readfile(&input, argv[1]);
    if (input_length == 0) {
        return 1;
    }
    printf("%s\n", input);
    printf("........ TOKENIZING INPUT ........\n");
    struct CompilerError error = { NULL, 1, 1 };
    Tokens *tokens = linkedlist_new();
    struct Lexer lexer = {
        &error,
        input_length,
        input,
        0,
        tokens
    };
    // TODO: delete this, it's only used for debugging
    debug_lexer = &lexer;

    tokenize(&lexer);
    if (lexer.error->message) {
        print_error(lexer.error);
        return 1;
    }
    print_lexer(&lexer);

    printf("........ PARSING AST FROM TOKENS ........\n");
    struct Parser parser = { lexer.tokens->head, &lexer };
    struct Value *value = parse_expr(&parser);
    if (value) {
        print_value(value);
        printf("\n");
    } else {
        printf("no value. sad\n");
    }

    return 0;
}
