#include <assert.h>
#include "common.h"
#include "printers.h"
#include "parser.h"
#include "bytecode.h"
#include "vm.h"


int main(int argc, char *argv[])
{
    assert(sizeof(void *) == sizeof(long));
    assert(sizeof(void *) == sizeof(double));

    void *instructions[100];
    int offset;
    
    if (argc > 2) {
        printf("Expected input expression\n");
        return -1;
    } else if (argc == 2) {
        struct Parser parser;
        struct Expr expr; 
        parser.expr = &expr;
        parser.error = calloc(sizeof(char), ERR_MSG_LEN); // probably enough characters

        int is_success = parse(&parser, argv[1]);
        if (!is_success) {
            printf("Error parsing expression: %s\n", parser.error);
            return -1;
        }
        printf("expression: ");
        print_expr(&expr, 0);
        putchar('\n');
        offset = compile(instructions, &expr, 0);
        instructions[offset] = (void *) RET;
    } else {
        // instructions[0] = 4
        // instructions[1] = 5
        // instructions[2]
        printf("needs arguments or something\n");
        exit(0);
    }
    print_instructions(instructions, offset+1);

    long ret = vm_execute(instructions);
    printf("ret: %li\n", ret);
    return 0;
}

