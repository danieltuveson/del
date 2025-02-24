#include "del.h"
#include "common.h"
#include "ast.h"
#include "linkedlist.h"
#include "ffi.h"

struct ForeignFunction *ffi_lookup_ff(struct Globals *globals, Symbol symbol)
{
    struct ForeignFunction *ff = NULL;
    linkedlist_vforeach(ff, globals->foreign_function_table) {
        if (ff->symbol == symbol) {
            return ff;
        }
    }
    return NULL;
}

Type convert_ffi_type(enum DelForeignType dtype)
{
    Type type = TYPE_UNDEFINED;
    switch (dtype) {
        case DEL_UNDEFINED:
            type = TYPE_UNDEFINED;
            break;
        case DEL_NULL:
            type = TYPE_NULL;
            break;
        case DEL_INT:
            type = TYPE_INT;
            break;
        case DEL_FLOAT:
            type = TYPE_FLOAT;
            break;
        case DEL_BOOL:
            type = TYPE_BOOL;
            break;
        case DEL_BYTE:
            type = TYPE_BYTE;
            break;
        case DEL_OBJECT:
        default:
            assert(false);
            break;
    }
    return type;
}

static void validate_ffs(struct Globals *globals, char *ff_name)
{
    struct ForeignFunction *ff = NULL;
    linkedlist_vforeach(ff, globals->foreign_function_table) {
        if (strcmp(ff->function_name, ff_name) == 0) {
            printf("Error: Cannot register the same function twice\n");
            assert(false);
        }
    }
}

void ffi_register_function(struct Globals *globals, void *context, DelForeignFunctionCall function,
        char *ff_name, enum DelForeignType rettype, Types *types) 
{
    validate_ffs(globals, ff_name);
    Symbol symbol = add_symbol(globals, ff_name, strlen(ff_name));
    struct ForeignFunction *ff = allocator_malloc(globals->allocator, sizeof(*ff));
    ff->symbol = symbol;
    ff->function_name = ff_name;
    ff->return_type   = convert_ffi_type(rettype);
    ff->arg_types     = types;
    ff->context       = context;
    ff->function      = function;
    linkedlist_append(globals->foreign_function_table, ff);
}

bool ffi_register_functions(struct Globals *globals)
{ 
    struct ForeignFunction *ff = NULL;
    linkedlist_vforeach(ff, globals->foreign_function_table) {
        struct ForeignFunctionBody *ffb = allocator_malloc(globals->allocator, sizeof(*ffb));
        ffb->context = ff->context;
        ffb->function = ff->function;
        Type rettype = ff->return_type;
        struct TopLevelDecl *tld = new_tld_foreign_fundef(globals, ff->symbol, rettype,
                ff->arg_types, ffb);
        linkedlist_append(globals->ast, tld);
    }
    return true;
}

