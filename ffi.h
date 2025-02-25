#ifndef FFI_H
#define FFI_H

#include "ast.h"
#include "del.h"

struct ForeignFunctionBody {
    bool is_yielding;
    void *context;
    union DelForeignValue (*function)(union DelForeignValue *, void *);
};

struct ForeignFunction {
    Symbol symbol;
    char *function_name;
    bool is_yielding;
    Type return_type;
    void *context;
    Types *arg_types;
    DelForeignFunctionCall function;
};

Type convert_ffi_type(enum DelForeignType dtype);
struct ForeignFunction *ffi_lookup_ff(struct Globals *globals, Symbol symbol);
void ffi_register_function(struct Globals *globals, void *context, bool is_yielding,
        DelForeignFunctionCall function, char *ff_name, enum DelForeignType rettype, Types *types);
bool ffi_register_functions(struct Globals *globals);

#endif

