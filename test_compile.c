#include "vm.h"
#include "printers.h"

#define init_test_state() \
    uint64_t instructions[INSTRUCTIONS_SIZE]; \
    struct CompilerContext cc_ = { instructions, 0, NULL }; \
    struct CompilerContext *cc = &cc_

static void test_next(void)
{
    init_test_state();
    assert(next(cc) == 0);
    assert(next(cc) == 1);
    assert(next(cc) == 2);
    assert(next(cc) == 3);
    assert(next(cc) == 4);
    printf("next test passed\n");
}

static void test_load(void)
{
    init_test_state();
    assert(cc->offset == 0);
    load(cc, 12);
    assert(cc->instructions[0] == 12);
    assert(cc->offset == 1);
    load(cc, 1013);
    assert(cc->instructions[1] == 1013);
    assert(cc->offset == 2);
    load(cc, 666);
    assert(cc->instructions[2] == 666);
    assert(cc->offset == 3);
    printf("load test passed\n");
}

static void test_compile_int(void)
{
    init_test_state();
    assert(cc->offset == 0);
    compile_value(cc, new_integer(1101));
    assert(cc->offset == 2);
    assert(cc->instructions[0] == PUSH);
    assert(cc->instructions[1] == 1101);
    assert(cc->offset == 2);

    // test runtime
    load(cc, POP);
    load(cc, EXIT);
    assert(1101 == vm_execute(instructions));
    printf("compile_int test passed\n");
}

static void test_compile_loadsym(void)
{
    init_test_state();
    assert(cc->offset == 0);
    compile_value(cc, new_symbol(1245));
    assert(cc->offset == 2);
    assert(cc->instructions[0] == LOAD);
    assert(cc->instructions[1] == 1245);
    assert(cc->offset == 2);
    printf("compile_loadsym test passed\n");
}

static void test_compile_add(void)
{
    init_test_state();
    compile_value(cc, new_expr(bin_plus(new_integer(4), new_integer(66))));
    assert(cc->instructions[0] == PUSH);
    assert(cc->instructions[1] == 4);
    assert(cc->instructions[2] == PUSH);
    assert(cc->instructions[3] == 66);
    assert(cc->instructions[4] == ADD);
    assert(cc->offset == 5);

    // test runtime
    load(cc, POP);
    load(cc, EXIT);
    assert(70 == vm_execute(instructions));
    printf("compile_add test passed\n");
}

static void test_compile_string(void)
{
    uint64_t packed;
    uint64_t tmp;
    init_test_state();
    assert(cc->offset == 0);
    compile_value(cc, new_string("hello, world"));

    assert(cc->instructions[0] == PUSH);
    tmp = 'h';
    packed = tmp;
    tmp = 'e';
    packed = packed | (tmp << 8);
    tmp = 'l';
    packed = packed | (tmp << 16);
    packed = packed | (tmp << 24);
    tmp = 'o';
    packed = packed | (tmp << 32);
    tmp = ',';
    packed = packed | (tmp << 40);
    tmp = ' ';
    packed = packed | (tmp << 48);
    tmp = 'w';
    packed = packed | (tmp << 56);
    assert(cc->instructions[1] == packed);

    assert(cc->instructions[2] == PUSH);
    tmp = 'o';
    packed = tmp;
    tmp = 'r';
    packed = packed | (tmp << 8);
    tmp = 'l';
    packed = packed | (tmp << 16);
    tmp = 'd';
    packed = packed | (tmp << 24);
    assert(cc->instructions[3] == packed);

    assert(cc->instructions[4] == PUSH);
    assert(cc->instructions[5] == 2);

    assert(cc->instructions[6] == PUSH_HEAP);

    assert(cc->offset == 7);
    printf("compile_string test passed\n");
}

static void test_compile_set(void)
{
    init_test_state();
    assert(cc->offset == 0);
    compile_statement(cc, new_set(1000, new_integer(123)));

    assert(cc->instructions[0] == PUSH);
    assert(cc->instructions[1] == 123);
    assert(cc->instructions[2] == PUSH);
    assert(cc->instructions[3] == 1000);
    assert(cc->instructions[4] == DEF);
    assert(cc->offset == 5);

    // test runtime
    load(cc, LOAD);
    load(cc, 1000);
    load(cc, POP);
    load(cc, EXIT);
    assert(123 == vm_execute(instructions));
    printf("compile_set test passed\n");
}

static void run_tests(void)
{
    printf("start of compilation tests\n");
    test_next();
    test_load();
    test_compile_int();
    test_compile_loadsym();
    test_compile_add();
    test_compile_string();
    test_compile_set();
    printf("end of compilation tests\n");
}
 
#undef init_test_state

