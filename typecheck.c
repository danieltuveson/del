#include "typecheck.h"
#include "ast.h"
// #include "printers.h"

#define ALL_GOOD 1
#define BAD_STUFF 0

#define find_open_loc(i, table, length, symbol)\
    for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)

struct ClassTable {
    size_t size;
    struct Class *table;
};

struct FunctionTable {
    size_t size;
    struct FunDef *table;
};

struct Scope {
    Definitions *definitions;
    struct Scope *parent;
};

struct TypeCheckerContext {
    struct FunDef *enclosing_func;
    struct FunctionTable *fun_table;
    struct ClassTable *cls_table;
    struct Scope *scope;
};

static int typecheck_statements(struct TypeCheckerContext *context, Statements *stmts);
static int typecheck_statement(struct TypeCheckerContext *context, struct Statement *stmt);
static int typecheck_funcall(struct TypeCheckerContext *context, struct FunCall *funcall);

#define lookup(table, length, symbol)\
    uint64_t i = symbol % length;\
    for (uint64_t count = 0; count < length; count++) {\
        if (table[i].name == symbol) {\
            return &(table[i]);\
        }\
        i = i == length - 1 ? 0 : i + 1;\
    }\
    return NULL

struct Class *lookup_class(struct Class *table, uint64_t length, Symbol symbol)
{
    lookup(table, length, symbol);
}

struct FunDef *lookup_fun(struct FunDef *table, uint64_t length, Symbol symbol)
{
    lookup(table, length, symbol);
}

#undef lookup

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
    if (scope->parent != NULL) {
        return lookup_var(scope->parent, name);
    }
    return NULL;
}

static void add_class(struct ClassTable *ct, struct Class *cls)
{
    uint64_t loc;
    find_open_loc(loc, ct->table, ct->size, cls->name);
    struct Class *clst = &ct->table[loc];
    clst->name = cls->name;
    clst->definitions = cls->definitions;
    // Eventually we'll need to figure out what to do with methods... maybe a table for each?
}

static void add_function(struct FunctionTable *ft, struct FunDef *fundef)
{
    uint64_t loc;
    find_open_loc(loc, ft->table, ft->size, fundef->name);
    struct FunDef *new_fundef = &ft->table[loc];
    new_fundef->name = fundef->name;
    // new_fundef->rettype = TYPE_UNDEFINED;
    new_fundef->rettype = fundef->rettype;
    new_fundef->args = fundef->args;
    new_fundef->stmts = fundef->stmts;
}

// Add declared types to ast without typechecking
static void add_types(TopLevelDecls *tlds, struct ClassTable *clst, struct FunctionTable *ft)
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

static Type typecheck_value(struct TypeCheckerContext *context, struct Value *val);

static Type typecheck_expression(struct TypeCheckerContext *context, struct Expr *expr)
{
    const Type type_left      = typecheck_value(context, expr->val1);
    const Type type_right = (expr->val2 == NULL)
                          ? TYPE_UNDEFINED
                          : typecheck_value(context, expr->val2);
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

static Type typecheck_value(struct TypeCheckerContext *context, struct Value *val)
{
    struct Definition *def = NULL;
    switch (val->type) {
        case VTYPE_STRING: return TYPE_STRING;
        case VTYPE_INT:    return TYPE_INT;
        case VTYPE_FLOAT:  return TYPE_FLOAT;
        case VTYPE_BOOL:   return TYPE_BOOL;
        case VTYPE_SYMBOL:
            def = lookup_var(context->scope, val->symbol);
            if (def == NULL || def->type == TYPE_UNDEFINED) {
                printf("Error: '%s' is not defined\n", lookup_symbol(val->symbol));
                return TYPE_UNDEFINED;
            } else {
                return def->type;
            }
        case VTYPE_EXPR: return typecheck_expression(context, val->expr);
        case VTYPE_FUNCALL: return typecheck_funcall(context, val->funcall);
    }
}

static int typecheck_set(struct TypeCheckerContext *context, struct Statement *stmt)
{
    Type type;
    if (stmt->set->is_define) {
        struct Definition *def = malloc(sizeof(*def));
        def->name = stmt->set->symbol;
        def->type = TYPE_UNDEFINED;
        add_var(context->scope, def);
    }
    struct Definition *def = lookup_var(context->scope, stmt->set->symbol);
    if (def == NULL) {
        printf("Symbol '%s' is used before it is declared\n", lookup_symbol(stmt->set->symbol));
        return 0;
    }
    type = typecheck_value(context, stmt->set->val);
    if (type == TYPE_UNDEFINED) {
        return 0;
    } else if (type != def->type && def->type != TYPE_UNDEFINED) {
        char *name = lookup_symbol(stmt->set->symbol);
        char *variable_type = lookup_symbol(def->type);
        char *value_type = lookup_symbol(type);
        char *err = "Error: %s is of type %s, cannot set to type %s\n";
        printf(err, name, variable_type, value_type);
        return 0;
    }
    def = malloc(sizeof(*def));
    def->name = stmt->set->symbol;
    def->type = type;
    add_type(context->scope, def);
    return 1;
}

static void scope_vars(struct Scope *scope, Definitions *defs)
{
    for (; defs != NULL; defs = defs->next) {
        struct Definition *def = defs->value;
        add_var(scope, def);
    }
}

static int typecheck_if(struct TypeCheckerContext *context, struct IfStatement *if_stmt)
{
    enter_scope(&context->scope);
    Type t0 = typecheck_value(context, if_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in if condition\n");
        return 0;
    }
    int ret = typecheck_statements(context, if_stmt->if_stmts);
    if (if_stmt->else_stmts != NULL) {
        ret = ret && typecheck_statements(context, if_stmt->else_stmts);
    }
    exit_scope(&context->scope);
    return ret;
}

static int typecheck_while(struct TypeCheckerContext *context, struct While *while_stmt)
{
    Type t0 = typecheck_value(context, while_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean in while condition\n");
        return 0;
    }
    int ret = typecheck_statements(context, while_stmt->stmts);
    return ret;
}

static int typecheck_for(struct TypeCheckerContext *context, struct For *for_stmt)
{
    int ret = typecheck_statement(context, for_stmt->init);
    if (!ret) return 0;
    Type t0 = typecheck_value(context, for_stmt->condition);
    if (t0 == TYPE_UNDEFINED) {
        return 0;
    } else if (t0 != TYPE_BOOL) {
        printf("Error: expected boolean as second parameter as for loop condition\n");
        return 0;
    }
    ret = typecheck_statement(context, for_stmt->increment);
    if (!ret) return 0;
    ret = typecheck_statements(context, for_stmt->stmts);
    return ret;
}

static int typecheck_return(struct TypeCheckerContext *context, struct Value *val)
{
    char *errmsg = "Error: function return type is %s but returning value of type %s\n";
    Type rettype = context->enclosing_func->rettype;
    if (val == NULL) {
        if (rettype == TYPE_UNDEFINED) {
            return 1;
        } else {
            printf(errmsg, "void", lookup_symbol(rettype));
            return 0;
        }
    }
    Type val_type = typecheck_value(context, val);
    if (val_type == TYPE_UNDEFINED) {
        return 0;
    } else if (rettype != val_type) {
        printf(errmsg, lookup_symbol(rettype), lookup_symbol(val_type));
        return 0;
    }
    return 1;
}

static int typecheck_funcall(struct TypeCheckerContext *context, struct FunCall *funcall)
{
    struct FunctionTable *ft = context->fun_table;
    struct FunDef *fundef = lookup_fun(ft->table, ft->size, funcall->funname);
    if (fundef == NULL) {
        printf("Error: no function named %s\n", lookup_symbol(funcall->funname));
        return 0;
    } else if (fundef->args->length != funcall->args->length) {
        printf("Error: %s expects %llu arguments but got %llu\n", lookup_symbol(fundef->name),
                fundef->args->length, funcall->args->length);
        return 0;
    }
    Values *vals = funcall->args;
    Definitions *defs = fundef->args;
    for (uint64_t i = 0; i < funcall->args->length; i++) {
        struct Value *val = vals->value;
        struct Definition *fun_arg_def = defs->value;
        Type val_type = typecheck_value(context, val);
        if (val_type != fun_arg_def->type) {
            printf("Error: expected argument %llu to %s to be of type %s, but got argument of "
                    "type %s\n", i + 1, lookup_symbol(fundef->name),
                    lookup_symbol(fun_arg_def->type),
                    lookup_symbol(val_type));
            return 0;
        }
        vals = vals->next;
        defs = defs->next;
    }
    return 1;
}

static int typecheck_statement(struct TypeCheckerContext *context, struct Statement *stmt)
{
    switch (stmt->type) {
        case STMT_SET:     return typecheck_set(context, stmt);
        case STMT_LET:     scope_vars(context->scope, stmt->let); return 1;
        case STMT_IF:      return typecheck_if(context, stmt->if_stmt);
        case STMT_WHILE:   return typecheck_while(context, stmt->while_stmt);
        case STMT_FOR:     return typecheck_for(context, stmt->for_stmt);
        case STMT_FUNCALL: return typecheck_funcall(context, stmt->funcall);
        case STMT_RETURN:  return typecheck_return(context, stmt->ret);
        case STMT_FOREACH: printf("Error: not implemented\n");
                           return 0;
    }
}

static int typecheck_statements(struct TypeCheckerContext *context, Statements *stmts)
{
    for (; stmts != NULL; stmts = stmts->next) {
        struct Statement *stmt = stmts->value;
        if (!typecheck_statement(context, stmt)) {
            return 0;
        }
    }
    return 1;
}

static int typecheck_fundef(struct TypeCheckerContext *context, struct FunDef *fundef)
{
    struct Scope *scope = NULL;
    enter_scope(&scope);
    context->enclosing_func = fundef;
    context->scope = scope;
    scope_vars(context->scope, fundef->args);
    int ret = typecheck_statements(context, fundef->stmts);
    exit_scope(&scope);
    return ret;
}

static int typecheck_tld(struct TypeCheckerContext *context, struct TopLevelDecl *tld)
{
    switch (tld->type) {
        case TLD_TYPE_CLASS:
            // typecheck_class(context, tld->cls);
            printf("Error: not implemented\n");
            return 0;
        case TLD_TYPE_FUNDEF:
            if (!typecheck_fundef(context, tld->fundef)) {
                return 0;
            }
            break;
    }
    return 1;
}

static int typecheck_tlds(struct TypeCheckerContext *context, TopLevelDecls *tlds)
{
    for (;tlds != NULL; tlds = tlds->next) {
        if (!typecheck_tld(context, tlds->value)) {
            return 0;
        }
    }
    return 1;
}

int typecheck(struct Ast *ast, struct Class *clst, struct FunDef *ft)
{
    struct ClassTable class_table = { ast->class_count, clst };
    struct FunctionTable function_table = { ast->function_count, ft };
    add_types(ast->ast, &class_table, &function_table);
    // for (uint64_t i = 0; i < ast->class_count; i++) {
    //     print_class(&(clst[i]), 0);
    //     printf("\n");
    // }
    // for (uint64_t i = 0; i < ast->function_count; i++) {
    //     print_fundef(&(ft[i]), 0, 0);
    //     printf("\n");
    // }
    struct TypeCheckerContext context = { NULL, &function_table, &class_table, NULL };
    return typecheck_tlds(&context, ast->ast);
}

#undef find_open_loc

