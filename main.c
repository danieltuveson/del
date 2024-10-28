#include "common.h"
#include "allocator.h"
#include "readfile.h"
#include "lexer.h"
#include "error.h"
#include "ast.h"
#include "parser.h"
#include "typecheck.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"
#include "vector.h"

int main()
{
    DelValue value;
    struct Vector *vector = vector_new(4, 32);
    for (int64_t i = 0; i < 16; i++)  {
        vector_print(vector);
        printf("===============================\n");
        value.integer = i;
        if (vector_append(&vector, value) == NULL) {
            printf("Error: could not allocate %ld elements\n", i + 1);
            break;
        }
    }
    vector_print(vector);
    vector_shrink(&vector, 16);
    // for (int64_t i = 0; i < 16; i++)  {
    //     vector_print(vector);
    //     printf("===============================\n");
    //     value = vector_pop(&vector);
    //     printf("Popped value: %ld\n", value.integer);
    // }
    vector_print(vector);
    vector_free(vector);
    return 0;
}

int _main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        goto FAIL;
    }

#if DEBUG
    printf("........ READING FILE : %s ........\n", argv[1]);
#endif
    struct FileContext file = { argv[1], 0, NULL };
    if (!readfile(&file)) {
        printf("Error: could not read contents of empty file\n");
        goto FAIL;
    }
    globals.file = &file;
#if DEBUG
    printf("%s\n", globals.file->input);
    print_memory_usage();

    printf("........ TOKENIZING INPUT ........\n");
#endif
    struct Lexer lexer;
    lexer_init(&lexer, false);
    if (!tokenize(&lexer)) {
        // print_error(&(lexer.error));
        printf("Error at line %d column %d: %s\n", lexer.error.line_number, lexer.error.column_number,
                lexer.error.message);
        goto FAIL;
    }
    globals.lexer = &lexer;
#if DEBUG
    print_lexer(&lexer);
    print_memory_usage();
#endif

#if DEBUG
    printf("........ PRINTING ALL SYMBOLS ........\n");
    linkedlist_foreach(lnode, globals.symbol_table->head) {
        printf("symbol: '%s'\n", (char *) lnode->value);
    }

    printf("........ PARSING AST FROM TOKENS ........\n");
#endif
    struct Parser parser = { lexer.tokens->head, &lexer };
    globals.parser = &parser;
    globals.ast = parse_tlds(&parser);

    if (!globals.ast) {
        error_print();
        goto FAIL;
    }
#if DEBUG
    print_tlds(globals.ast);
    printf("\n");
    print_memory_usage();

    printf("````````````````` CODE `````````````````\n");
#endif
    struct Class *clst = allocator_malloc(globals.class_count * sizeof(*clst));
    struct FunDef *ft = allocator_malloc(globals.function_count * sizeof(*ft));
    struct ClassTable class_table = { globals.class_count, clst };
    struct FunctionTable function_table = { globals.function_count, ft };
    DelValue instructions[INSTRUCTIONS_MAX];
    struct CompilerContext cc = { instructions, 0, NULL, &class_table };
    assert(globals.ast != NULL);
#if DEBUG
    print_tlds(globals.ast);
    printf("\n");

    printf("`````````````` TYPECHECK ```````````````\n");
#endif
    if (typecheck(&class_table, &function_table)) {
#if DEBUG
        printf("program has typechecked\n");
#endif
    } else {
#if DEBUG
        printf("program failed to typecheck\n");
#endif
        goto FAIL;
    }
#if DEBUG
    printf("`````````````` COMPILE ```````````````\n");
#endif
    compile(&cc, globals.ast);
#if DEBUG
    printf("\n");
    printf("````````````` INSTRUCTIONS `````````````\n");
    printf("function table:\n");
    print_ft(cc.funcall_table);
    printf("\n");
    print_instructions(&cc);
    printf("\n");

    printf("`````````````` EXECUTION ```````````````\n");
#endif
    ret = vm_execute(instructions);
#if DEBUG
    printf("ret: %d\n", ret);
#endif

    ret = EXIT_SUCCESS;
FAIL:
    allocator_freeall();
    return ret;
}
