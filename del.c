#include "del.h"
#include "common.h"
#include "allocator.h"
#include "readfile.h"
#include "lexer.h"
#include "error.h"
#include "ast.h"
#include "parser.h"
#include "typecheck.h"
#include "compiler.h"
#include "printers.h"
#include "vector.h"


static bool read_and_compile(struct Vector **instructions_ptr, Allocator allocator,
        char *filename)
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
    // print_instructions(cc.instructions);
    // printf("\n");
    return true;
}

DelAllocator del_allocator_new(void)
{
    DelAllocator da = (DelAllocator) allocator_new();
    return da;
}

void del_allocator_freeall(DelAllocator da)
{
    Allocator allocator = (Allocator) da;
    allocator_freeall(allocator);
}

void del_instructions_free(DelInstructions del_instructions)
{
    struct Vector *instructions = (struct Vector *) del_instructions;
    vector_free(instructions);
}

void del_vm_init(DelVM *del_vm, DelInstructions del_instructions)
{
    struct VirtualMachine *vm = malloc(sizeof(*vm));
    printf("Size of vm: %lu\n", sizeof(*vm));
    memset(vm, 0, sizeof(*vm));
    struct Vector *instructions = (struct Vector *) del_instructions;
    vm_init(vm, instructions->values);
    *del_vm = (DelVM) vm;
}

void del_vm_execute(DelVM del_vm)
{
    struct VirtualMachine *vm = (struct VirtualMachine *) del_vm;
    vm_execute(vm);
}

enum VirtualMachineStatus del_vm_status(DelVM del_vm)
{
    struct VirtualMachine *vm = (struct VirtualMachine *) del_vm;
    return vm->status;
}

void del_vm_free(DelVM del_vm)
{
    struct VirtualMachine *vm = (struct VirtualMachine *) del_vm;
    vm_free(vm);
    free(vm);
}

DelInstructions del_read_and_compile(DelAllocator del_allocator, char *filename)
{
    struct Vector *instructions;
    Allocator allocator = (Allocator) del_allocator;
    if (read_and_compile(&instructions, allocator, filename)) {
        DelInstructions del_instructions = (DelInstructions) instructions;
        return del_instructions;
    }
    return 0;
}
