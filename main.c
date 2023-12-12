#include <assert.h>
#include "common.h"
#include "printers.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"


int main(int argc, char *argv[])
{
    assert(sizeof(void *) == sizeof(long));
    assert(sizeof(void *) == sizeof(double));

    void *instructions[100];
    int offset;
    struct Exprs *exprs;
    
    if (argc > 2) {
        printf("Expected input expression\n");
        return EXIT_FAILURE;
    } else if (argc == 2) {
        char *error = calloc(sizeof(char), ERR_MSG_LEN); // probably enough characters
        void *memory = init_allocator(strlen(argv[1])); 
        if (memory == NULL || error == NULL) {
            printf("Error could not allocate enough memory to parse input");
            return EXIT_FAILURE;
        }

        exprs = alloc(sizeof(struct Exprs));
        int is_success = parse(exprs, error, argv[1]);
        if (!is_success) {
            printf("Error parsing expression: %s\n", error);
            free(error);
            return EXIT_FAILURE;
        }
        printf("expression(s): ");
        print_exprs(exprs);
        putchar('\n');
        offset = compile(instructions, exprs, 0);
    } else {
        // instructions[0] = 4
        // instructions[1] = 5
        // instructions[2]
        printf("needs arguments or something\n");
        return EXIT_FAILURE;
    }
    free(exprs);
    print_instructions(instructions, offset+1);

    long ret = vm_execute(instructions);
    printf("ret: %li\n", ret);
    return EXIT_SUCCESS;
}

