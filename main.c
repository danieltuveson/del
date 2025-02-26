#include <stdio.h>
#include <string.h>
#include "del.h"
#include "printers.h"

// TODO: As an alternative to a REPL, add a "watch" flag (maybe '-w') that recompiles + reruns the
// code if any files have changed similar to ghcid.

DelProgram compile_with_args(DelCompiler compiler, int argc, char *argv[])
{
    if (argc < 2) {
        printf("Error: must supply an argument\n");
        printf("Example: `del examples/hello.del`\n");
        printf("Run `del -h` for a full list of command line options and example usage\n");
        return 0;
    } else if (strcmp(argv[1], "-h") == 0) {
        printf("Usage: del [options] [script]\n");
        printf("Options:\n");
        printf("  -e stuff   execute string 'stuff'\n");
        return 0;
    }
    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) {
            printf("Error: must supply a string to run\n");
            return 0;
        } else if (argc > 3) {
            printf("Error: too many arguments\n");
            return 0;
        }
        return del_compile_text(compiler, argv[2]);
    }
    if (argc < 2) {
        printf("Error: must supply a file to run\n");
        return 0;
    } else if (argc > 2) {
        printf("Error: too many arguments\n");
        return 0;
    }
    return del_compile_file(compiler, argv[1]);
}

#define IGNORE(val) (void)val

// Simplest example foreign function
union DelForeignValue hello_world(union DelForeignValue *arguments, void *context)
{
    IGNORE(arguments);
    IGNORE(context);
    printf("Hello, from the world of C!\n");
    DEL_NORETURN();
}

// Example foreign function that adds two integers and returns the result
union DelForeignValue add_ints(union DelForeignValue *arguments, void *context)
{
    IGNORE(context);
    int64_t arg1 = arguments[0].integer;
    int64_t arg2 = arguments[1].integer;
    printf("Adding %li + %li from the FFI\n", arg1, arg2);
    union DelForeignValue del_val;
    del_val.integer = arg1 + arg2;
    return del_val;
}

// Example foreign function that adds two floats, returns the result, and makes the result
// available in a C context pointer
union DelForeignValue add_floats(union DelForeignValue *arguments, void *context)
{
    double arg1 = arguments[0].floating;
    double arg2 = arguments[1].floating;
    printf("Adding %f + %f from the FFI\n", arg1, arg2);
    union DelForeignValue del_val;
    del_val.floating = arg1 + arg2;
    *((float *)context) = del_val.floating;
    return del_val;
}

// Example del program
int main(int argc, char *argv[])
{
    // Init compiler
    DelCompiler compiler;
    del_compiler_init(&compiler, stderr);

    // Add foreign functions
    // del_register_yielding_function(compiler, NULL, hello_world, DEL_UNDEFINED);
    // del_register_function(compiler, NULL, add_ints, DEL_INT, DEL_INT, DEL_INT);

    // Set up any info that we want to read from foreign function
    // float del_val_context;
    // del_register_function(compiler, &del_val_context, add_floats,
    // DEL_FLOAT, DEL_FLOAT, DEL_FLOAT);

    // Compile
    DelProgram program = compile_with_args(compiler, argc, argv);
    if (!program) {
        del_compiler_free(compiler);
        return EXIT_FAILURE;
    }
    del_compiler_free(compiler);

    // Run
    DelVM vm;
    del_vm_init(&vm, stdout, stderr, program);
    del_vm_execute(vm);
    // while (del_vm_status(vm) == DEL_VM_STATUS_YIELD) {
    //     printf("Resuming after yield...\n");
    //     del_vm_execute(vm);
    // }
    // printf("Finished!\n");
    // printf("Number we added earlier: %f\n", del_val_context);

    if (del_vm_status(vm) == DEL_VM_STATUS_ERROR) {
        del_vm_free(vm);
        del_program_free(program);
        return EXIT_FAILURE;
    }

    del_vm_free(vm);
    del_program_free(program);
    return EXIT_SUCCESS;
}

