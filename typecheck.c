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

static int typecheck_statements(struct Scope *scope, Statements *stmts);
static int typecheck_statement(struct Scope *scope, struct Statement *stmt);

void print_scope(struct Scope *scope)
{
    if (scope->parent != NULL) {
        print_scope(scope->parent);
    } else {
        printf("(Top Level)");
    }
    printf("<- { ");
    for (Definitions *defs = scope->definitions; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
        printf("%s: %s", lookup_symbol(def->name), lookup_symbol(def->type));
        if (defs->next != NULL) {
            printf(", ");
        }
    }
    printf(" }\n");
}

static void enter_scope(struct Scope **current)
{
    struct Scope *scope = malloc(sizeof(*scope));
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

static int add_type(struct Scope *current, struct Definition *def)
{
    for (Definitions *defs = current->definitions; defs != NULL; defs = defs->next) {
        struct Definition *lookup_def = defs->value;
        if (lookup_def->name == def->name) {
            lookup_def->type = def->type;
            return 1;
        }
    }
    if (current->parent == NULL) {
        return 0;
    } else {
        return add_type(current->parent, def);
    }
}

static struct Definition *lookup_var(struct Scope *scope, Symbol name)
{
    for (Definitions *defs = scope->definitions; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
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
    ft->rettype = TYPE_UNDEFINED;
    ft->args = fundef->args;
    ft->stmts = fundef->stmts;
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

static Type typecheck_expression(struct Scope *scope, struct Expr *expr)
{
    const Type type_left      = typecheck_value(scope, expr->val1);
    const Type type_right = (expr->val2 == NULL)
                          ? TYPE_UNDEFINED
                          : typecheck_value(scope, expr->val2);
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
            if (are_both_bool) {
                return TYPE_BOOL;
            } else {
                printf("Error: boolean operator has non-boolean operands\n");
                return TYPE_UNDEFINED;
            }
        case OP_EQEQ:
        case OP_NOT_EQ:
            if (are_both_int || are_both_float || are_both_bool || are_both_string) {
                return TYPE_BOOL;
            } else {
                printf("Error: mismatched types on equality operands\n");
                return TYPE_UNDEFINED;
            }
        case OP_GREATER_EQ:
        case OP_GREATER:
        case OP_LESS_EQ:
        case OP_LESS:
            if (are_both_int || are_both_float) {
                return TYPE_BOOL;
            } else {
                printf("Error: mismatched types on comparison operands\n");
                return TYPE_UNDEFINED;
            }
        case OP_PLUS:
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_float) {
                return TYPE_FLOAT;
            } else if (are_both_string) {
                return TYPE_STRING;
            } else {
                printf("Error: mismatched types for '+' operands\n");
                return TYPE_UNDEFINED;
            }
        case OP_MINUS:
        case OP_STAR:
        case OP_SLASH:
            if (are_both_int) {
                return TYPE_INT;
            } else if (are_both_float) {
                return TYPE_FLOAT;
            } else {
                printf("Error: mismatched types for numeric operator's operands\n");
                return TYPE_UNDEFINED;
            }
        case OP_UNARY_PLUS:
        case OP_UNARY_MINUS:
            if (left_is_int && right_is_null) {
                return TYPE_INT;
            } else if (left_is_float && right_is_null) {
                return TYPE_FLOAT;
            } else {
                return TYPE_UNDEFINED;
            }
    };
}

static Type typecheck_value(struct Scope *scope, struct Value *val)
{
    struct Definition *def = NULL;
    switch (val->type) {
        case VTYPE_STRING: return TYPE_STRING;
        case VTYPE_INT:    return TYPE_INT;
        case VTYPE_FLOAT:  return TYPE_FLOAT;
        case VTYPE_BOOL:   return TYPE_BOOL;
        case VTYPE_SYMBOL:
            def = lookup_var(scope, val->symbol);
            if (def == NULL || def->type == TYPE_UNDEFINED) {
                printf("Error: '%s' is not defined\n", lookup_symbol(val->symbol));
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

static int typecheck_set(struct Scope *scope, struct Statement *stmt)
{
    Type type;
    if (stmt->set->is_define) {
        //struct Definition def = { .name = stmt->set->symbol, .type = TYPE_UNDEFINED };
        struct Definition *def = malloc(sizeof(*def));
        def->name = stmt->set->symbol;
        def->type = TYPE_UNDEFINED;
        add_var(scope, def);
    }
    struct Definition *def = lookup_var(scope, stmt->set->symbol);
    if (def == NULL) {
    //if (lookup_var(scope, stmt->set->symbol)) {
        printf("Symbol '%s' is used before it is declared\n", lookup_symbol(stmt->set->symbol));
        return 0;
    }
    type = typecheck_value(scope, stmt->set->val);
    if (type == TYPE_UNDEFINED) {
        return 0;
    } else if (type != def->type && def->type != TYPE_UNDEFINED) {
        char *name = lookup_symbol(stmt->set->symbol);
        char *variable_type = lookup_symbol(def->type);
        char *value_type = lookup_symbol(type);
        char *err = "Error: %s is of type %s, cannot set to type %s\n";
        printf(err, name, variable_type, value_type);
        return 0;
    } else {
        //struct Definition def = { .name = stmt->set->symbol, .type = type };
        struct Definition *def = malloc(sizeof(*def));
        def->name = stmt->set->symbol;
        def->type = type;
        add_type(scope, def);
    }
    return 1;
}

static void scope_let_vars(struct Scope *scope, Definitions *defs)
{
    for (; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
        add_var(scope, def);
    }
}

static int typecheck_if(struct Scope *scope, struct IfStatement *if_stmt)
{
    Type t0 = typecheck_value(scope, if_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in if condition\n");
        return 0;
    }
    int ret = typecheck_statements(scope, if_stmt->if_stmts);
    if (if_stmt->else_stmts != NULL) {
        ret = ret && typecheck_statements(scope, if_stmt->else_stmts);
    }
    return ret;
}

// struct Value *condition;
// Statements *stmts;
static int typecheck_while(struct Scope *scope, struct While *while_stmt)
{
    Type t0 = typecheck_value(scope, while_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in while condition\n");
        return 0;
    }
    int ret = typecheck_statements(scope, while_stmt->stmts);
    return ret;
}

// struct Statement *init;
// struct Value *condition;
// struct Statement *increment;
// Statements *stmts;
static int typecheck_for(struct Scope *scope, struct For *for_stmt)
{
    int ret = typecheck_statement(scope, for_stmt->init);
    if (!ret) return 0;
    Type t0 = typecheck_value(scope, for_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean as second parameter as for loop condition\n");
        return 0;
    }
    ret = typecheck_statement(scope, for_stmt->increment);
    if (!ret) return 0;
    ret = typecheck_statements(scope, for_stmt->stmts);
    return ret;
}

static int typecheck_statement(struct Scope *scope, struct Statement *stmt)
{
    // switch (stmt->type) {
    //     case STMT_SET:    compile_set(cc, stmt->set);          break;
    //     case STMT_RETURN: compile_return(cc, stmt->ret);       break;
    //     case STMT_IF:     compile_if(cc, stmt->if_stmt);       break;
    //     case STMT_WHILE:  compile_while(cc, stmt->while_stmt); break;
    //     default: printf("Error cannot compile statement type: not implemented\n"); break;
    // }
    switch (stmt->type) {
        case STMT_SET:    return typecheck_set(scope, stmt);
        case STMT_RETURN: return 0;
        case STMT_LET:    scope_let_vars(scope, stmt->let); return 1;
        case STMT_IF:     return typecheck_if(scope, stmt->if_stmt);
        case STMT_WHILE:  return typecheck_while(scope, stmt->while_stmt);
        case STMT_FOR:    return typecheck_for(scope, stmt->for_stmt);
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
        if (!typecheck_statement(scope, stmt)) {
            return 0;
        }
    }
    return 1;
}

int typecheck(struct Ast *ast, struct Class *clst, struct FunDef *ft)
{
    add_types(ast->ast, clst, ft);
    for (uint64_t i = 0; i < ast->class_count; i++) {
        print_class(&(clst[i]), 0);
        printf("\n");
    }
    for (uint64_t i = 0; i < ast->function_count; i++) {
        print_fundef(&(ft[i]), 0, 0);
        printf("\n");
    }
    int ret = 1;
    struct Scope *scope = NULL;
    enter_scope(&scope);
    struct FunDef *main = lookup(ft, ast->function_count, ast->entrypoint);
    ret = typecheck_statements(scope, main->stmts);
    exit_scope(&scope);
    return ret;
}

#undef find_open_loc

