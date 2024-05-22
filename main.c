// #include <stdio.h>
#include <assert.h>
#include "common.h"
#include "printers.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "ast.h"
#include "typecheck.h"

// struct ClassTable {
//     size_t size;
//     struct Class *table;
// };
// 
// struct FunctionTable {
//     size_t size;
//     struct FunDef *table;
// };


int main(int argc, int *argv[])
{
    int ret = parse();
    if (ret != 0) {
        printf("parse error\n");
        return EXIT_FAILURE;
    }
    struct Class *clst = calloc(ast.class_count, sizeof(*clst));
    struct FunDef *ft = calloc(ast.function_count, sizeof(*ft));
    struct ClassTable class_table = { ast.class_count, clst };
    struct FunctionTable function_table = { ast.function_count, ft };
    uint64_t instructions[INSTRUCTIONS_SIZE];
    struct CompilerContext cc = { instructions, 0, NULL, &class_table };
    printf("````````````````` CODE `````````````````\n");
    assert(ast.ast != NULL);
    print_tlds(ast.ast);
    printf("\n");

    printf("`````````````` TYPECHECK ```````````````\n");
    if (typecheck(&ast, &class_table, &function_table)) {
        printf("program has typechecked\n");
    } else {
        printf("program failed to typecheck\n");
        return EXIT_FAILURE;
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
    return EXIT_SUCCESS;
}

