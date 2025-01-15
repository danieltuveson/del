#include "common.h"
#include "del.h"
#include "printers.h"

// Example del program
int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        return EXIT_FAILURE;
    }

    // Compile
    DelAllocator allocator = del_allocator_new();
    DelInstructions instructions = del_read_and_compile(allocator, argv[1]);
    if (!instructions) {
        del_allocator_freeall(allocator);
        return EXIT_FAILURE;
    }
    del_allocator_freeall(allocator);

    // Run
    DelVM vm;
    del_vm_init(&vm, instructions);
    del_vm_execute(vm);
    if (del_vm_status(vm) == VM_STATUS_ERROR) {
        del_vm_free(vm);
        del_instructions_free(instructions);
        return EXIT_FAILURE;
    }
    del_vm_free(vm);
    del_instructions_free(instructions);
    return EXIT_SUCCESS;
}

