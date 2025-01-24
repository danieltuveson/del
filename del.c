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


static bool read_and_compile(struct Program **program, Allocator allocator, char *filename)
{
    struct Globals globals = { {0}, 0, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL };
    globals.allocator = allocator;

#if DEBUG_TEXT
    printf("........ READING FILE : %s ........\n", filename);
#endif
    struct FileContext file = { filename, 0, NULL };
    if (!readfile(&globals, &file)) {
        printf("Error: could not read contents of empty file\n");
        return false;
    }

    globals.file = &file;
#if DEBUG_TEXT
    printf("%s\n", globals.file->input);
    print_memory_usage(globals.allocator);
#endif

#if DEBUG_LEXER
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
#if DEBUG_LEXER
    print_lexer(&globals, globals.lexer);
    print_memory_usage(globals.allocator);
#endif

#if DEBUG_LEXER
    printf("........ PRINTING ALL SYMBOLS ........\n");
    linkedlist_foreach(lnode, globals.symbol_table->head) {
        printf("symbol: '%s'\n", (char *) lnode->value);
    }
#endif

#if DEBUG_PARSER
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
#if DEBUG_PARSER
    print_tlds(&globals, globals.ast);
    printf("\n");
    print_memory_usage(globals.allocator);

    printf("````````````````` CODE `````````````````\n");
    print_tlds(&globals, globals.ast);
    printf("\n");

#endif
#if DEBUG_TYPECHECKER
    printf("`````````````` TYPECHECK ```````````````\n");
#endif
    if (typecheck(&globals)) {
#if DEBUG_TYPECHECKER
        printf("program has typechecked\n");
#endif
    } else {
#if DEBUG_TYPECHECKER
        printf("program failed to typecheck\n");
#endif
        return false;
    }
#if DEBUG_COMPILER
    printf("`````````````` COMPILE ```````````````\n");
#endif
    compile(&globals, globals.ast);
    *program = malloc(sizeof(**program));
    (*program)->instructions = globals.cc->instructions;
    (*program)->string_count = globals.cc->string_count;
    (*program)->string_pool = globals.cc->string_pool;
#if DEBUG_COMPILER
    printf("\n");
    printf("````````````` INSTRUCTIONS `````````````\n");
    printf("function table:\n");
    print_ft(&globals, globals.cc->funcall_table);
    printf("\n");
    print_instructions(globals.cc);
    printf("\n");
#endif
    return true;
}

/*
typedef DelValue del_function(DelValue *values, size_t length) DelFunction;

void del_register_functions(void)
{
}
*/

/*

list (length + array) of type-value pairs (del_type, del_value)

del_type is a string for the name of the type. del_value


 */

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

void del_program_free(DelProgram del_program)
{
    struct Program *program = (struct Program *) del_program;
    vector_free(program->instructions);
    for (size_t i = 0; i < program->string_count; i++) {
        free(program->string_pool[i]);
    }
    if (program->string_pool != NULL) free(program->string_pool);
    free(program);
}

void del_vm_init(DelVM *del_vm, DelProgram del_program)
{
    struct VirtualMachine *vm = malloc(sizeof(*vm));
    memset(vm, 0, sizeof(*vm));
    struct Program *program = (struct Program *) del_program;
    vm_init(vm, program->instructions->values, program->string_pool);
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

DelProgram del_read_and_compile(DelAllocator del_allocator, char *filename)
{
    struct Program *program = NULL;
    Allocator allocator = (Allocator) del_allocator;
    if (read_and_compile(&program, allocator, filename)) {
        DelProgram del_program = (DelProgram) program;
        return del_program;
    }
    return 0;
}

