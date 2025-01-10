#include "common.h"
#include "del.h"

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
    DO_WE_HAVE_EXPECT();
    return EXIT_SUCCESS;

    // struct Vector *instructions;
    // if (!read_and_compile(&instructions, allocator, argv[1])) {
    //     allocator_freeall(allocator);
    //     return EXIT_FAILURE;
    // }
    //allocator_freeall(allocator);

    // struct VirtualMachine vm = {};
    // vm_init(&vm, instructions->values);
    // vm_execute(&vm);
    // if (vm.status == VM_STATUS_ERROR) {
    //     vm_free(&vm);
    //     vector_free(instructions);
    //     return EXIT_FAILURE;
    // }
    // // printf("Executed %lu operations for program %s\n", vm.iterations, argv[1]);
    // vector_free(instructions);
    // return EXIT_SUCCESS;
}

// int main(int argc, char *argv[])
// {
//     if (argc != 2) {
//         printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
//         return false;
//     }
// 
//     Allocator allocator = allocator_new();
//     struct Vector *instructions;
//     if (!read_and_compile(&instructions, allocator, argv[1])) {
//         allocator_freeall(allocator);
//         return EXIT_FAILURE;
//     }
//     allocator_freeall(allocator);
// 
//     struct VirtualMachine vm = {};
//     vm_init(&vm, instructions->values);
//     vm_execute(&vm);
//     if (vm.status == VM_STATUS_ERROR) {
//         vm_free(&vm);
//         vector_free(instructions);
//         return EXIT_FAILURE;
//     }
//     // printf("Executed %lu operations for program %s\n", vm.iterations, argv[1]);
//     vector_free(instructions);
//     return EXIT_SUCCESS;
// }

// int _main()
// {
//     DelValue value;
//     struct Vector *vector = vector_new(4, 32);
//     for (int64_t i = 0; i < 16; i++)  {
//         vector_print(vector);
//         printf("===============================\n");
//         value.integer = i;
//         if (vector_append(&vector, value) == NULL) {
//             printf("Error: could not allocate %ld elements\n", i + 1);
//             break;
//         }
//     }
//     vector_print(vector);
//     vector_shrink(&vector, 16);
//     // for (int64_t i = 0; i < 16; i++)  {
//     //     vector_print(vector);
//     //     printf("===============================\n");
//     //     value = vector_pop(&vector);
//     //     printf("Popped value: %ld\n", value.integer);
//     // }
//     vector_print(vector);
//     vector_free(vector);
//     return 0;
// }
