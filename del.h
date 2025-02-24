#ifndef DEL_H
#define DEL_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

enum DelVirtualMachineStatus {
    DEL_VM_STATUS_ERROR = 0, 
    DEL_VM_STATUS_COMPLETED = 1, 
    DEL_VM_STATUS_PAUSE = 2
};

typedef intptr_t DelProgram;
typedef intptr_t DelVM;
typedef intptr_t DelCompiler;
typedef intptr_t DelForeignFunction;

// Foreign Function Interface
enum DelForeignType {
    DEL_UNDEFINED,
    DEL_NULL,
    DEL_INT,
    DEL_FLOAT,
    DEL_BOOL,
    DEL_BYTE,
    DEL_OBJECT
};

struct DelForeignObject {
    char *type_name;
    void *c_struct;
};

union DelForeignValue {
    int64_t integer;
    double floating;
    bool boolean;
    char byte;
    struct DelForeignObject *object;
};

typedef union DelForeignValue (*DelForeignFunctionCall)(union DelForeignValue *, void *);

// Del compiler functions
void del_compiler_init(DelCompiler *compiler, FILE *ferr);
void del_compiler_free(DelCompiler compiler);
// void del_register_function(DelCompiler compiler, DelForeignFunction ff);
void del_register_function_helper(DelCompiler compiler, void *context,
        DelForeignFunctionCall function, char *ff_name, int arg_count, ...);
DelProgram del_compile_text(DelCompiler compiler, char *program_text);
DelProgram del_compile_file(DelCompiler compiler, char *filename);
void del_program_free(DelProgram del_program);
 
#define DEL_ARG_COUNT(...) \
    (sizeof((enum DelForeignType[]){__VA_ARGS__})/sizeof(enum DelForeignType))

#define del_register_function(compiler, context, function, ...) \
    (del_register_function_helper(compiler, context, function, #function, DEL_ARG_COUNT(__VA_ARGS__) - 1, __VA_ARGS__))

// Del runtime functions
void del_vm_init(DelVM *del_vm, FILE *fout, FILE *ferr, DelProgram del_program);
void del_vm_execute(DelVM del_vm);
void del_vm_free(DelVM del_vm);
enum DelVirtualMachineStatus del_vm_status(DelVM del_vm);

#endif
