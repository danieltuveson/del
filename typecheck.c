#include "typecheck.h"
#include "ast.h"
#include "printers.h"

#define ALL_GOOD 1
#define BAD_STUFF 0

#define find_open_loc(i, table, length, symbol)\
    for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)

struct Class *lookup_class(struct Class *table, Symbol symbol)
{
    uint64_t i = symbol % ast.class_count;
    for (; table[i].name != symbol; i = i == ast.class_count - 1 ? 0 : i + 1);
    return &(table[i]);
}

struct FunDef *lookup(struct FunDef *table, uint64_t length, Symbol symbol)
{
    uint64_t i = symbol % length;
    for (; table[i].name != symbol; i = i == length - 1 ? 0 : i + 1);
    return &(table[i]);
}

struct Scope {
    Definitions *definitions;
    struct Scope *parent;
};

static void enter_scope(struct Scope **current)
{
    struct Scope *scope = malloc(sizeof(struct Scope));
    scope->definitions = NULL;
    scope->parent = *current;
    *current = scope;
}

static void exit_scope(struct Scope **current)
{
    *current = (*current)->parent;
}

static void add_var(struct Scope *current, struct Definition *def)
{
    if (current->definitions == NULL) {
        current->definitions = new_list(def);
    } else {
        current->definitions = append(current->definitions, def);
    }
}

static struct Definition *lookup_var(struct Scope *scope, Symbol name)
{
    Definitions *defs = scope->definitions;
    for (; defs != NULL; defs = defs->next) {
        struct Definition *def = (struct Definition *) defs->value;
        if (def->name == name) {
            return def;
        }
    }
    return NULL;
}

static void add_class(struct Class *table, struct Class *cls)
{
    uint64_t loc;
    find_open_loc(loc, table, ast.class_count, cls->name);
    struct Class *clst = &table[loc];
    clst->name = cls->name;
    clst->definitions = cls->definitions;
    // Eventually we'll need to figure out what to do with methods... maybe a table for each?
}

static void add_function(struct FunDef *table, struct FunDef *fundef)
{
    uint64_t loc;
    find_open_loc(loc, table, ast.function_count, fundef->name);
    struct FunDef *ft = &table[loc];
    ft->name = fundef->name;
    ft->args = fundef->args;
}

// Add declared types to ast without typechecking
static void add_types(TopLevelDecls *tlds, struct Class *clst, struct FunDef *ft)
{
    for (; tlds != NULL; tlds = tlds->next) {
        struct TopLevelDecl *tld = tlds->value;
        if (tld->type == TLD_TYPE_FUNDEF) {
            add_function(ft, tld->fundef);
        } else {
            add_class(clst, tld->cls);
        }
    }
}

static Type typecheck_value(struct Scope *scope, struct Value *val);

static int typecheck_expression(struct Scope *scope, struct Expr *expr)
{
    const Type type_left      = typecheck_value(scope, expr->val1);
    const Type type_right = (expr->val2 == NULL)
                          ? TYPE_UNDEFINED
                          : typecheck_value(scope, expr->val1);
    const int left_is_int     = type_left  == TYPE_INT;
    const int right_is_int    = type_right == TYPE_INT;
    const int left_is_float   = type_left  == TYPE_FLOAT;
    const int right_is_float  = type_right == TYPE_FLOAT;
    const int right_is_null   = type_right == TYPE_UNDEFINED;
    const int are_both_int    = left_is_int   && right_is_int;
    const int are_both_float  = left_is_float && right_is_float;
    const int are_both_bool   = (type_left  == TYPE_BOOL) && (type_right == TYPE_BOOL);
    const int are_both_string = (type_left  == TYPE_STRING) && (type_right == TYPE_STRING);
    // const int are_both_other  = expr->val1 right_is_string;
    // have to find a way to handle non-primitives
    switch (expr->op) {
        case OP_OR:
        case OP_AND:
            return are_both_bool;
        case OP_EQEQ:
        case OP_NOT_EQ:
            return are_both_int || are_both_float || are_both_bool || are_both_string;
        case OP_GREATER_EQ:
        case OP_GREATER:
        case OP_LESS_EQ:
        case OP_LESS:
        case OP_PLUS:
        case OP_MINUS:
        case OP_STAR:
        case OP_SLASH:
            return are_both_int || are_both_float;
        case OP_UNARY_PLUS:
        case OP_UNARY_MINUS:
            return (left_is_int && right_is_null) || (left_is_float && right_is_null);
    };
}

static Type typecheck_value(struct Scope *scope, struct Value *val)
{
    struct Definition *def = NULL;
    switch (val->type) {
        case VTYPE_STRING:
            return TYPE_STRING;
        case VTYPE_INT:
            return TYPE_INT;
        case VTYPE_FLOAT:
            return TYPE_FLOAT;
        case VTYPE_BOOL:
            return TYPE_BOOL;
        case VTYPE_SYMBOL:
            def = lookup_var(scope, val->symbol);
            if (def == NULL) {
                return TYPE_UNDEFINED;
            } else {
                return def->type;
            }
        case VTYPE_EXPR:
            return typecheck_expression(scope, val->expr);
        case VTYPE_FUNCALL:
            printf("unknown type\n");
            return 0;
    }
}
static int typecheck_statement(struct Statement *stmt)
{
    // switch (stmt->type) {
    //     case STMT_SET:    compile_set(cc, stmt->set);          break;
    //     case STMT_RETURN: compile_return(cc, stmt->ret);       break;
    //     case STMT_IF:     compile_if(cc, stmt->if_stmt);       break;
    //     case STMT_WHILE:  compile_while(cc, stmt->while_stmt); break;
    //     default: printf("Error cannot compile statement type: not implemented\n"); break;
    // }
    switch (stmt->type) {
        case STMT_SET:
        case STMT_RETURN:
            return 0;
        case STMT_LET:
        case STMT_IF:
        case STMT_WHILE:
        case STMT_FOR:
        case STMT_FOREACH:
        case STMT_FUNCALL:
            printf("cannot typecheck statement of this type\n");
            return 0;
    }
}

static int typecheck_functioncall(struct FunCall *funcall)
{
    return 0;
}

static int typecheck_statements(struct Scope *scope, Statements *stmts)
{
    for (; stmts != NULL; stmts = stmts->next) {
        struct Statement *stmt = (struct Statement *) stmts->value;
    }
    return 0;
}

int typecheck(struct Ast *ast, struct Class *clst, struct FunDef *ft)
{
    add_types(ast->ast, clst, ft);
    // for (uint64_t i = 0; i < ast.class_count; i++) {
    //     print_class(&(clst[i]), 0);
    //     printf("\n");
    // }
    // for (uint64_t i = 0; i < ast.function_count; i++) {
    //     print_fundef(&(ft[i]), 0, 0);
    //     printf("\n");
    // }
    int ret = 1;
    struct Scope *scope = NULL;
    enter_scope(&scope);
    struct FunDef *main = lookup(ft, ast->function_count, ast->entrypoint);
    typecheck_statements(scope, main->stmts);
    exit_scope(&scope);
    return ret;
}

#undef find_open_loc

