#include "typecheck.h"
#include "ast.h"
#include "printers.h"

#define ALL_GOOD 1
#define BAD_STUFF 0

#define find_open_loc(i, table, length, symbol)\
    for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)

// struct ClassType {
//     Symbol name;
//     uint64_t count;
//     Type *types;
// };
// 
// struct FunctionType {
//     Symbol name;
//     uint64_t count;
//     Type *types;
// };
// 
// struct Scope {
//     Definitions *definitions;
//     struct Scope *parent;
// };

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
static void add_class(struct ClassType *table, struct Class *cls)
{
    uint64_t loc;
    find_open_loc(loc, table, ast.class_count, cls->name);
    struct ClassType *clst = &table[loc];
    clst->name = cls->name;
    clst->count = cls->definitions->length;
    clst->types = calloc(clst->count, sizeof(Type));
    for (uint64_t i = 0; i < clst->count; i++) {
        clst->types[i] = ((struct Definition *) cls->definitions->value)->type;
        cls->definitions = cls->definitions->next;
    }
    // Eventually we'll need to figure out what to do with methods... maybe a table for each?
}

static void add_function(struct FunctionType *table, struct FunDef *fundef)
{
    uint64_t loc;
    find_open_loc(loc, table, ast.function_count, fundef->name);
    struct FunctionType *ft = &table[loc];
    ft->name = fundef->name;
    ft->count = fundef->args->length;
    ft->types = calloc(ft->count, sizeof(Type));
    for (uint64_t i = 0; i < ft->count; i++) {
        ft->types[i] = ((struct Definition *) fundef->args->value)->type;
        fundef->args = fundef->args->next;
    }
}

// Add declared types to ast without typechecking
static void add_types(TopLevelDecls *tlds, struct ClassType *class_table,
        struct FunctionType *function_table)
{
    for (; tlds != NULL; tlds = tlds->next) {
        struct TopLevelDecl *tld = tlds->value;
        if (tld->type == TLD_TYPE_FUNDEF) {
            add_function(function_table, tld->fundef);
        } else {
            add_class(class_table, tld->cls);
        }
    }
}

static int typecheck_expression(struct Expr *expr);

static int typecheck_value(struct Value *val)
{
    switch (val->type) {
        case VTYPE_STRING:
        case VTYPE_INT:
        case VTYPE_FLOAT:
        case VTYPE_BOOL:
            return 1;
        case VTYPE_SYMBOL:
            printf("unknown type\n");
            return 0;
        case VTYPE_EXPR:
            return typecheck_expression(val->expr);
        case VTYPE_FUNCALL:
            printf("unknown type\n");
            return 0;
    }
}

static int typecheck_expression(struct Expr *expr)
{
    const int left_is_int     = expr->val1->type == TYPE_INT;
    const int right_is_int    = expr->val2->type == TYPE_INT;
    const int left_is_float   = expr->val1->type == TYPE_FLOAT;
    const int right_is_float  = expr->val2->type == TYPE_FLOAT;
    const int left_is_bool    = expr->val1->type == TYPE_BOOL;
    const int right_is_bool   = expr->val2->type == TYPE_BOOL;
    const int left_is_string  = expr->val1->type == TYPE_STRING;
    const int right_is_string = expr->val2->type == TYPE_STRING;
    const int right_is_null   = expr->val2 == NULL;
    const int are_both_int    = left_is_int    && right_is_int;
    const int are_both_float  = left_is_float  && right_is_float;
    const int are_both_bool   = left_is_bool   && right_is_bool;
    const int are_both_string = left_is_string && right_is_string;
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
}

static void typecheck_tlds(TopLevelDecls *tlds, struct ClassType *class_table, 
        struct FunctionType *function_table)
{
    for (; tlds != NULL; tlds = tlds->next) {
        struct TopLevelDecl *tld = tlds->value;
        if (tld->type == TLD_TYPE_FUNDEF) {
            if (typecheck_functioncall(tld
        } else {
            // add_class(class_table, tld->cls);
        }
    }
    return 1;
}

int typecheck(void)
{
    struct ClassType *class_table = calloc(ast.class_count, sizeof(struct ClassType));
    struct FunctionType *function_table = calloc(ast.function_count, sizeof(struct FunctionType));
    add_types(ast.ast, class_table, function_table);
    print_function_table(function_table, ast.function_count);
    print_class_table(class_table, ast.class_count);
    typecheck_tlds(ast.ast, class_table, function_table);
    return ALL_GOOD;
}

#undef find_open_loc

