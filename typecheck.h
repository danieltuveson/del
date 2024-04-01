#ifndef TYPECHECK_H
#define TYPECHECK_H

#include "common.h"
#include "ast.h"


// #define find_open_loc(i, table, length, symbol)\
//     for (i = symbol % length; table[i].name != 0; i = i == length - 1 ? 0 : i + 1)
// 
// static Definitions *lookup(struct FunctionType *table, uint64_t length, Symbol symbol)
// {
//     // uint64_t length = ast.function_count;
//     uint64_t i = symbol % length;
//     for (; table[i].name != symbol; i = i == length - 1 ? 0 : i + 1);
//     return table[i].definitions;
// }
// 
// struct ClassType {
//     Symbol name;
//     uint64_t count;
//     Definitions *definitions;
// };
// 
// struct FunctionType {
//     Symbol name;
//     uint64_t count;
//     Definitions *args;
// };
// 
// struct Scope {
//     Definitions *definitions;
//     struct Scope *parent;
// };
// struct ClassType {
//     Symbol name;
//     uint64_t count;
//     Definitions *definitions;
// };
// 
// struct FunctionType {
//     Symbol name;
//     uint64_t count;
//     Definitions *definitions;
// };

struct Class *lookup_class(struct Class *table, uint64_t length, Symbol symbol);
int typecheck(struct Ast *ast, struct Class *clst, struct FunDef *ft);

#endif
