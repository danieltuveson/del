#ifndef PARSER_H
#define PARSER_H
#define SYMBOL_SIZE 24
#define ERR_MSG_LEN 100

enum ExprType {
    VALUE,
    SYMBOL,
    EXPRESSION
};

struct BinaryExpr;

struct Expr {
    enum ExprType type;
    union {
        long value;
        char *symbol;
        struct BinaryExpr *binexpr;
    };
};

enum BinaryOp {
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION,
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,
    LEQUAL,
    GEQUAL,
    DEFINE
};

struct BinaryExpr {
    enum BinaryOp op;
    struct Expr *expr1;
    struct Expr *expr2;
};

struct Parser {
    struct Expr *expr;
    char *error;
};

int parse(struct Parser *parser, char *input);

#endif
