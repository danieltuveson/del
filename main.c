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

int _main()
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

bool read_and_compile(struct Vector **instructions_ptr, Allocator allocator, char *filename)
{
    struct Globals globals = { {0}, 0, NULL, NULL, NULL, NULL, 0, 0, 0, NULL, NULL };
    globals.allocator = allocator;

#if DEBUG
    printf("........ READING FILE : %s ........\n", filename);
#endif
    struct FileContext file = { filename, 0, NULL };
    if (!readfile(&globals, &file)) {
        printf("Error: could not read contents of empty file\n");
        return false;
    }

    globals.file = &file;
#if DEBUG
    printf("%s\n", globals.file->input);
    print_memory_usage(globals.allocator);

    printf("........ TOKENIZING INPUT ........\n");
#endif
    struct Lexer lexer;
    lexer_init(&globals, &lexer, false);
    globals.lexer = &lexer;
    if (!tokenize(&globals)) {
        printf("Error at line %d column %d: %s\n",
                globals.lexer->error.line_number,
                globals.lexer->error.column_number,
                globals.lexer->error.message);
        return false;
    }
#if DEBUG
    print_lexer(&globals, globals.lexer);
    print_memory_usage(globals.allocator);
#endif

#if DEBUG
    printf("........ PRINTING ALL SYMBOLS ........\n");
    linkedlist_foreach(lnode, globals.symbol_table->head) {
        printf("symbol: '%s'\n", (char *) lnode->value);
    }

    printf("........ PARSING AST FROM TOKENS ........\n");
#endif
    // struct Parser parser = { globals.lexer.tokens->head, &lexer };
    // globals.parser = &parser;
    globals.parser = globals.lexer->tokens->head;
    globals.ast = parse_tlds(&globals);

    if (!globals.ast) {
        error_print(&globals);
        return false;
    }
#if DEBUG
    print_tlds(&globals, globals.ast);
    printf("\n");
    print_memory_usage(globals.allocator);

    printf("````````````````` CODE `````````````````\n");
#endif
    struct Class *clst = allocator_malloc(globals.allocator, globals.class_count * sizeof(*clst));
    struct FunDef *ft = allocator_malloc(globals.allocator, globals.function_count * sizeof(*ft));
    struct ClassTable class_table = { globals.class_count, clst };
    struct FunctionTable function_table = { globals.function_count, ft };
    struct CompilerContext cc = { NULL, NULL, &class_table };
    assert(globals.ast != NULL);
#if DEBUG
    print_tlds(&globals, globals.ast);
    printf("\n");

    printf("`````````````` TYPECHECK ```````````````\n");
#endif
    if (typecheck(&globals, &class_table, &function_table)) {
#if DEBUG
        printf("program has typechecked\n");
#endif
    } else {
#if DEBUG
        printf("program failed to typecheck\n");
#endif
        return false;
    }
#if DEBUG
    printf("`````````````` COMPILE ```````````````\n");
#endif
    globals.cc = &cc;
    compile(&globals, globals.ast);
    *instructions_ptr = globals.cc->instructions;
#if DEBUG
    printf("\n");
    printf("````````````` INSTRUCTIONS `````````````\n");
    printf("function table:\n");
    print_ft(&globals, cc.funcall_table);
    printf("\n");
    print_instructions(cc.instructions);
    printf("\n");
#endif
    return true;
}

int main(int argc, char *argv[])
{
    // if (argc < 2) {
    if (argc != 2) {
        printf("Error: expecting input file\nExample usage:\n./del hello_world.del\n");
        return false;
    } else if (argc > 10) {
        printf("Error: too many input files\n");
        return false;
    }

    Allocator allocator = allocator_new();
    struct Vector *instructions;
    if (!read_and_compile(&instructions, allocator, argv[1])) {
        allocator_freeall(allocator);
        return EXIT_FAILURE;
    }
    allocator_freeall(allocator);

    struct VirtualMachine vm = {};
    vm_init(&vm, instructions->values);
    int ret = vm_execute(&vm);
    if (ret == VM_STATUS_ERROR) {
        vm_free(&vm);
        vector_free(instructions);
        return EXIT_FAILURE;
    }

    vm_free(&vm);
    vector_free(instructions);
    return EXIT_SUCCESS;

#if 0
    DelValue **instructions_list = calloc(argc - 1, sizeof(**instructions_list));
    for (int i = 1; i < argc; i++) {
        struct Vector *instructions = vector_new(128, INSTRUCTIONS_MAX);
        printf("reading file %d\n", i);
        Allocator allocator = allocator_new();
        if (!read_and_compile(instructions, allocator, argv[i])) {
            allocator_freeall(allocator);
            return EXIT_FAILURE;
        }
        printf("compiling instructions %d\n", i);
        instructions_list[i-1] = instructions->values;
        allocator_freeall(allocator);
    }

#if DEBUG
    printf("`````````````` EXECUTION ```````````````\n");
#endif
    struct VirtualMachine **vms = calloc(argc - 1, sizeof(**vms));
    for (int i = 0; i < argc - 1; i++) {
        printf("initializing vm %d\n", i+1);
        struct VirtualMachine vm = vms + i;
        // DelValue *instructions = instructions_list[i]->values;
        vm_init(vm, instructions_list[i]);
    }
    bool is_finished[10] = {0};
    bool all_done = false;
    while (!all_done) {
        all_done = true;
        for (int i = 0; i < argc - 1; i++) {
            if (is_finished[i]) {
                continue;
            }
            // printf("running file %d\n", i+1);
            struct VirtualMachine *vm = vms + i;
            // DelValue *instructions = instructions_list[i];
            // struct VirtualMachine vm = {0};
            // vm_init(vm, instructions);
            int ret = vm_execute(vm);
            // printf("\n==== VM PAUSED ====\n");
            // printf("instruction status: %u\n", vm->status);
            // printf("instruction iterations: %lu\n", vm->iterations);
#if DEBUG
            printf("ret: %d\n", ret);
#endif
            is_finished[i] = vm->status != VM_STATUS_PAUSE;
            all_done = all_done && is_finished[i];
        }
    }
    // allocator_freeall(allocator);
    free(vms);
    free(instructions_list);
    return EXIT_SUCCESS;
#endif
}

