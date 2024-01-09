#include <assert.h>
#include "common.h"
#include "printers.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "ast.h"


// void *memory;
// struct Exprs *exprs;
struct Heap heap = { NULL, 0, NULL };

// int read(char *input)
// {
//     memory = init_allocator(strlen(input)); 
//     if (memory == NULL) {
//         goto err_no_mem; }
//     char *error = calloc(sizeof(char), ERR_MSG_LEN); // probably enough characters
//     if (error == NULL) {
//         free(memory);
//         goto err_no_mem;
//     }
// 
//     exprs = alloc(sizeof(struct Exprs));
//     int is_success = parse(exprs, error, input);
//     if (!is_success) {
//         printf("Error parsing expression: %s\n", error);
//         free(error);
//         free(memory);
//         return 0;
//     }
//     return 1;
// 
// err_no_mem:
//     printf("Error could not allocate enough memory to parse input");
//     return 0;
// }
// 
// void eval(void)
// {
// }
// 
// void print(void)
// {
// }

// int repl(void **instructions)
// {
//     int offset;
//     size_t size = 100;
//     char *buff = malloc(sizeof(char) * size);
//     
//     while (1) {
//         memset(buff, 0, size);
//         printf("> ");
//         fflush(stdout);
// 
//         // Read input text
//         int c = fgetc(stdin);
//         if (c == '\0') {
//             free(buff);
//             return EXIT_FAILURE;
//         } else if (c != '(') {
//             ungetc(c, stdin);
//             getline(&buff, &size, stdin);
//         } else {
//             ungetc(c, stdin);
//             int paren_count = 0;
//             for (size_t i = 0; i < size && !(paren_count <= 0 && c == '\n'); i++) {
//                 c = fgetc(stdin);
//                 buff[i] = (char) c;
//                 if (feof(stdin)) break;
//             }
//         }
//         if (feof(stdin)) {
//             printf("\nfarewellllllll\n");
//             return EXIT_SUCCESS;
//         }
// 
//         // Parse
//         if (!read(buff)) {
//             continue;
//         }
// 
//         // Compile and execute
//         offset = compile(instructions, exprs, 0);
//         print_instructions(instructions, offset+1);
// 
//         long ret = vm_execute(&heap, instructions);
//         printf("%li\n", ret);
//     }
// }

int main(int argc, char *argv[])
{
    assert(sizeof(void *) == sizeof(long));
    assert(sizeof(void *) == sizeof(double));

    // if (argc > 2) {
    //     printf("Too many arguments\n");
    //     return EXIT_FAILURE;
    // } 

    // void *instructions[100];
    // int offset;
    // 
    // if (argc == 2) {
    //     tokenize
    //     // Parse expressions passed from command line
    //     if (!read(argv[1])) {
    //         return EXIT_FAILURE;
    //     }

    //     // Compile and execute
    //     offset = compile(instructions, exprs, 0);
    //     free(memory);
    //     print_instructions(instructions, offset+1);

    //     long ret = vm_execute(&heap, instructions);
    //     printf("ret: %li\n", ret);
    // } else if (argc == 1) {
    //     // Repl
    //     return repl(instructions);
    // } else {
    //     printf("needs arguments or something\n");
    //     return EXIT_FAILURE;
    // }

    int ret = parse();
    if (ret != 0) {
        printf("parse error\n");
    } else {
        void *instructions[100];
        assert(ast != NULL);
        printf("printing stuff\n");
        print_statements(ast);
        printf("done printing stuff\n");
        printf("\n");

        // struct Statement *stmt = ast->value;
        // assert(stmt->type == STMT_SET);
        int offset = compile(instructions, ast, 0);
        print_instructions(instructions, offset+1);

        long ret = vm_execute(&heap, instructions);
        print_heap(&heap);
        printf("ret: %li\n", ret);
    }
    return EXIT_SUCCESS;
}

