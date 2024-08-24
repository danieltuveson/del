// struct FunDef {
//     Symbol name;
//     Type rettype;
//     Definitions *args;
//     Statements *stmts;
// };
// 
// struct Definition {
//     size_t scope_offset;
//     Symbol name;
//     Type type;
// };
// 
// struct VirtualMachine {
//     struct StackFrames sfs;
//     struct Stack stack;
//     struct Heap heap;
//     uint64_t ip;
//     int iterations;
// };

typedef void (*RegisteredFunction)(struct VirtualMachine *, struct FunDef *);

struct ForeignFunction {
    Symbol name;
    Type rettype;
    size_t num_args;
    struct Definition *args;
    RegisteredFunction func;
};

void register_function(struct VirtualMachine *vm, Symbol name, Type rettype,
                       size_t num_args, struct Definition *args)
{
}