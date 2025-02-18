#include <stdio.h>
#include <string.h>
#include "del.h"
#include "printers.h"

// TODO: As an alternative to a REPL, add a "watch" flag (maybe '-w') that recompiles + reruns the
// code if any files have changed similar to ghcid.

DelProgram compile_with_args(int argc, char *argv[], DelAllocator allocator)
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
        return del_compile_text(allocator, stderr, argv[2]);
    }
    if (argc < 2) {
        printf("Error: must supply a file to run\n");
        return 0;
    } else if (argc > 2) {
        printf("Error: too many arguments\n");
        return 0;
    }
    return del_compile_file(allocator, stderr, argv[1]);
}

// Example del program
int main(int argc, char *argv[])
{
    // Compile
    DelAllocator allocator = del_allocator_new();
    DelProgram program = compile_with_args(argc, argv, allocator);
    if (!program) {
        del_allocator_freeall(allocator);
        return EXIT_FAILURE;
    }
    del_allocator_freeall(allocator);

    // Run
    DelVM vm;
    del_vm_init(&vm, stdout, stderr, program);
    del_vm_execute(vm);
    if (del_vm_status(vm) == VM_STATUS_ERROR) {
        del_vm_free(vm);
        del_program_free(program);
        return EXIT_FAILURE;
    }
    del_vm_free(vm);
    del_program_free(program);
    return EXIT_SUCCESS;
}

