#include <assert.h>
#include "common.h"
#include "printers.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"


void *memory;
struct Exprs *exprs;
struct Heap heap = { NULL, 0, NULL };

int read(char *input)
{
    memory = init_allocator(strlen(input)); 
    if (memory == NULL) {
        free(memory);
        goto err_no_mem; }
    char *error = calloc(sizeof(char), ERR_MSG_LEN); // probably enough characters
    if (error == NULL) {
        free(error);
        goto err_no_mem;
    }

    exprs = alloc(sizeof(struct Exprs));
    int is_success = parse(exprs, error, input);
    if (!is_success) {
        printf("Error parsing expression: %s\n", error);
        free(error);
        free(memory);
        return 0;
    }
    // printf("expression(s): ");
    // print_exprs(exprs);
    // putchar('\n');
    return 1;

err_no_mem:
    printf("Error could not allocate enough memory to parse input");
    return 0;
}

void eval(void)
{
}

void print(void)
{
}

int main(int argc, char *argv[])
{
    assert(sizeof(void *) == sizeof(long));
    assert(sizeof(void *) == sizeof(double));

    if (argc > 2) {
        printf("Too many arguments\n");
        return EXIT_FAILURE;
    } 

    void *instructions[100];
    int offset;
    
    if (argc == 2) {
        // Parse expressions passed from command line
        if (!read(argv[1])) {
            return EXIT_FAILURE;
        }

        // Compile and execute
        offset = compile(instructions, exprs, 0);
        free(memory);
        print_instructions(instructions, offset+1);

        long ret = vm_execute(&heap, instructions);
        printf("ret: %li\n", ret);
    } else if (argc == 1) {
        // Repl
        size_t size = 100;
        char *buff = malloc(sizeof(char) * size);
        
        while (1) {
            memset(buff, 0, size);
            printf("> ");
            fflush(stdout);

            // Read input text
            int c = fgetc(stdin);
            if (c == '\0') {
                free(buff);
                return EXIT_FAILURE;
            } else if (c != '(') {
                ungetc(c, stdin);
                getline(&buff, &size, stdin);
            } else {
                ungetc(c, stdin);
                int paren_count = 0;
                for (size_t i = 0; i < size && !(paren_count <= 0 && c == '\n'); i++) {
                    c = fgetc(stdin);
                    buff[i] = (char) c;
                    if (feof(stdin)) break;
                }
            }
            if (feof(stdin)) {
                printf("\nfarewellllllll\n");
                return EXIT_SUCCESS;
            }

            // Parse
            if (!read(buff)) {
                continue;
            }

            // if (exprs->expr->type != EXPRESSION && exprs->next == NULL) {
                // If symbol or value, print it without doing anything
                // print_expr(exprs->expr, 0);
            // } else {
                // Compile and execute
                offset = compile(instructions, exprs, 0);
                print_instructions(instructions, offset+1);
            // }

            long ret = vm_execute(&heap, instructions);
            printf("%li\n", ret);
        }
    } else {
        // instructions[0] = 4
        // instructions[1] = 5
        // instructions[2]
        printf("needs arguments or something\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

