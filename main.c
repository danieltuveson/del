#include "common.h"
#include "allocator.h"
#include "readfile.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "typecheck.h"
#include "compiler.h"
#include "vm.h"
#include "printers.h"


int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        goto FAIL;
    }

    printf("........ READING FILE : %s ........\n", argv[1]);
    char *input = NULL;
    long input_length = readfile(&input, argv[1]);
    if (input_length == 0) {
        printf("Error: could not read contents of empty file\n");
        goto FAIL;
    }
    printf("%s\n", input);
    print_memory_usage();

    printf("........ TOKENIZING INPUT ........\n");
    struct Lexer lexer;
    lexer_init(&lexer, input, input_length, false);
    if (!tokenize(&lexer)) {
        print_error(&(lexer.error));
        goto FAIL;
    }
    print_lexer(&lexer);
    print_memory_usage();


    printf("........ PRINTING ALL SYMBOLS ........\n");
    linkedlist_foreach(lnode, ast.symbol_table->head) {
        printf("symbol: '%s'\n", (char *) lnode->value);
    }

    printf("........ PARSING AST FROM TOKENS ........\n");
    struct Parser parser = { lexer.tokens->head, &lexer };
    TopLevelDecls *tlds = parse_tlds(&parser);
    if (tlds) {
        print_tlds(tlds);
        printf("\n");
    } else {
        printf("no value. sad\n");
        goto FAIL;
    }
    print_memory_usage();

    ast.ast = tlds;

    printf("````````````````` CODE `````````````````\n");
    struct Class *clst = allocator_malloc(ast.class_count * sizeof(*clst));
    struct FunDef *ft = allocator_malloc(ast.function_count * sizeof(*ft));
    struct ClassTable class_table = { ast.class_count, clst };
    struct FunctionTable function_table = { ast.function_count, ft };
    uint64_t instructions[INSTRUCTIONS_SIZE];
    struct CompilerContext cc = { instructions, 0, NULL, &class_table };
    assert(ast.ast != NULL);
    print_tlds(ast.ast);
    printf("\n");

    printf("`````````````` TYPECHECK ```````````````\n");
    if (typecheck(&ast, &class_table, &function_table)) {
        printf("program has typechecked\n");
    } else {
        printf("program failed to typecheck\n");
        goto FAIL;
    }
    printf("`````````````` COMPILE ```````````````\n");
    compile(&cc, ast.ast);
    printf("\n");
    printf("````````````` INSTRUCTIONS `````````````\n");
    printf("function table:\n");
    print_ft(cc.funcall_table);
    printf("\n");
    print_instructions(&cc);
    printf("\n");

    printf("`````````````` EXECUTION ```````````````\n");
    ret = vm_execute(instructions);
    printf("ret: %d\n", ret);

    ret = EXIT_SUCCESS;
FAIL:
    allocator_freeall();
    return ret;
}