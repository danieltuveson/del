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


int main(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        goto FAIL;
    }

    printf("........ READING FILE : %s ........\n", argv[1]);
    struct FileContext file = { argv[1], 0, NULL };
    if (!readfile(&file)) {
        printf("Error: could not read contents of empty file\n");
        goto FAIL;
    }
    globals.file = &file;
    printf("%s\n", globals.file->input);
    print_memory_usage();

    printf("........ TOKENIZING INPUT ........\n");
    struct Lexer lexer;
    lexer_init(&lexer, globals.file->input, globals.file->length, false);
    if (!tokenize(&lexer)) {
        // print_error(&(lexer.error));
        printf("Error at line %d column %d: %s\n", lexer.error.line_number, lexer.error.column_number,
                lexer.error.message);
        goto FAIL;
    }
    globals.lexer = &lexer;
    print_lexer(&lexer);
    print_memory_usage();

    printf("........ PRINTING ALL SYMBOLS ........\n");
    linkedlist_foreach(lnode, globals.symbol_table->head) {
        printf("symbol: '%s'\n", (char *) lnode->value);
    }

    printf("........ PARSING AST FROM TOKENS ........\n");
    struct Parser parser = { lexer.tokens->head, &lexer };
    globals.parser = &parser;
    globals.ast = parse_tlds(&parser);

    if (globals.ast) {
        print_tlds(globals.ast);
        printf("\n");
    } else {
        error_print();
        goto FAIL;
    }
    print_memory_usage();

    printf("````````````````` CODE `````````````````\n");
    struct Class *clst = allocator_malloc(globals.class_count * sizeof(*clst));
    struct FunDef *ft = allocator_malloc(globals.function_count * sizeof(*ft));
    struct ClassTable class_table = { globals.class_count, clst };
    struct FunctionTable function_table = { globals.function_count, ft };
    DelValue instructions[INSTRUCTIONS_SIZE];
    struct CompilerContext cc = { instructions, 0, NULL, &class_table };
    assert(globals.ast != NULL);
    print_tlds(globals.ast);
    printf("\n");

    printf("`````````````` TYPECHECK ```````````````\n");
    if (typecheck(&class_table, &function_table)) {
        printf("program has typechecked\n");
    } else {
        printf("program failed to typecheck\n");
        goto FAIL;
    }
    printf("`````````````` COMPILE ```````````````\n");
    compile(&cc, globals.ast);
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