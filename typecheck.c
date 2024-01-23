#include "typecheck.h"
#include "ast.h"

#define ALL_GOOD 1
#define BAD_STUFF 0


// /* Doublely linked list used for various types in the AST.
//  * Should always point to elements of the same type. */
// struct List;
// struct List {
//     void *value;
//     struct List *prev;
//     struct List *next;
// };
// typedef struct List Values;
// typedef struct List Statements;
// typedef struct List Dim;
// 
// // Global variable to hold ast of currently parsed ast
// // I would prefer to avoid globals, but I'm not sure how
// // to get Bison to return a value without one
// struct List *ast;
// 
int typecheck(void)
{
    return ALL_GOOD;
}
// 
// /* Printers. Currently used for debugging purposes but could be used for 
//  * building a formatter in the future */
// void print_expr(struct Expr *expr)
// {
//     printf("(");
//     switch (expr->op) {
//     case OP_OR:
//         print_value(expr->val1);
//         printf(" or ");
//         print_value(expr->val2);
//         break;
//     case OP_AND:
//         print_value(expr->val1);
//         printf(" and ");
//         print_value(expr->val2);
//         break;
//     case OP_EQEQ:
//         print_value(expr->val1);
//         printf(" == ");
//         print_value(expr->val2);
//         break;
//     case OP_NOT_EQ:
//         print_value(expr->val1);
//         printf(" <> ");
//         print_value(expr->val2);
//         break;
//     case OP_GREATER_EQ:
//         print_value(expr->val1);
//         printf(" >= ");
//         print_value(expr->val2);
//         break;
//     case OP_GREATER:
//         print_value(expr->val1);
//         printf(" > ");
//         print_value(expr->val2);
//         break;
//     case OP_LESS_EQ:
//         print_value(expr->val1);
//         printf(" <= ");
//         print_value(expr->val2);
//         break;
//     case OP_LESS:
//         print_value(expr->val1);
//         printf(" < ");
//         print_value(expr->val2);
//         break;
//     case OP_PLUS:
//         print_value(expr->val1);
//         printf(" + ");
//         print_value(expr->val2);
//         break;
//     case OP_MINUS:
//         print_value(expr->val1);
//         printf(" - ");
//         print_value(expr->val2);
//         break;
//     case OP_STAR:
//         print_value(expr->val1);
//         printf(" * ");
//         print_value(expr->val2);
//         break;
//     case OP_SLASH:
//         print_value(expr->val1);
//         printf(" / ");
//         print_value(expr->val2);
//         break;
//     case OP_UNARY_PLUS:
//         printf("+");
//         print_value(expr->val1);
//         break;
//     case OP_UNARY_MINUS:
//         printf("-");
//         print_value(expr->val1);
//         break;
//     }
//     printf(")");
// }
// 
// void print_value(struct Value *val)
// {
//     switch (val->type) {
//     case VTYPE_SYMBOL:
//         printf("%s", val->symbol);
//         break;
//     case VTYPE_STRING:
//         printf("%s", val->string);
//         break;
//     case VTYPE_INT:
//         printf("%ld", val->integer);
//         break;
//     case VTYPE_FLOAT:
//         printf("%f", val->floating);
//         break;
//     case VTYPE_BOOL:
//         printf("%ld", val->boolean);
//         break;
//     case VTYPE_EXPR:
//         print_expr(val->expr);
//         break;
//     case VTYPE_FUNCALL:
//         printf("%s", val->funcall->funname);
//         printf("(");
//         for (Values *vals = val->funcall->values; vals != NULL;
//                 vals = vals->next) {
//             print_value(vals->value);
//             if (vals->next != NULL) printf(", ");
//         }
//         printf(")");
//         break;
//     }
// }
// 
// static void print_statements_indent(Statements *stmts, int indent);
// 
// /* :) */
// static void left_pad(int indent) {
//     for (int i = 0; i < indent; i++) putchar(' ');
// }
// 
// static const int TAB_WIDTH = 4;
// 
// static void print_statement_indent(struct Statement *stmt, int indent)
// {
//     Dim *dim = NULL;
//     struct Definition *def = NULL;
//     left_pad(indent);
//     switch (stmt->type) {
//         case STMT_DIM:
//             dim = stmt->dim;
//             printf("dim ");
//             while (dim != NULL) {
//                 def = (struct Definition *) dim->value;
//                 printf("%s as ", def->name);
//                 switch (def->type) {
//                     case TYPE_INT:
//                         printf("int");
//                         break;
//                     case TYPE_FLOAT:
//                         printf("float");
//                         break;
//                     case TYPE_BOOL:
//                         printf("bool");
//                         break;
//                     case TYPE_STRING:
//                         printf("string");
//                         break;
//                 }
//                 if (dim->next) printf(", ");
//                 dim = dim->next;
//             }
//             break;
//         case STMT_SET:
//             printf("%s = ", stmt->set->symbol);
//             print_value(stmt->set->val);
//             break;
//         case STMT_IF:
//             printf("if ");
//             print_value(stmt->if_stmt->condition);
//             printf(" then\n");
//             print_statements_indent(stmt->if_stmt->if_stmts, indent + TAB_WIDTH);
//             if (stmt->if_stmt->else_stmts) {
//                 left_pad(indent);
//                 printf("else\n");
//                 print_statements_indent(stmt->if_stmt->else_stmts, indent + TAB_WIDTH);
//             }
//             printf("end if");
//             break;
//         case STMT_WHILE:
//             printf("while ");
//             print_value(stmt->while_stmt->condition);
//             printf("\n");
//             print_statements_indent(stmt->while_stmt->stmts, indent + TAB_WIDTH);
//             printf("end while");
//             break;
//         case STMT_FOR:
//         case STMT_FOREACH:
//         case STMT_FUNCTION_DEF:
//         case STMT_EXIT_WHILE:
//         case STMT_EXIT_FOR:
//         case STMT_EXIT_FUNCTION:
//             printf("print_statement not yet implemented for this type");
//             break;
//         case STMT_FUNCALL:
//             printf("%s", stmt->funcall->funname);
//             printf("(");
//             for (Values *vals = stmt->funcall->values; vals != NULL;
//                     vals = vals->next) {
//                 print_value(vals->value);
//                 if (vals->next != NULL) printf(", ");
//             }
//             printf(")");
//             break;
//     }
//     printf("\n");
// }
// 
// static void print_statements_indent(Statements *stmts, int indent)
// {
//     while (stmts != NULL)
//     {
//         print_statement_indent(stmts->value, indent);
//         stmts = stmts->next;
//     }
// }
// 
// void print_statement(struct Statement *stmt)
// {
//     print_statement_indent(stmt, 0);
// }
// 
// void print_statements(Statements *stmts)
// {
//     print_statements_indent(stmts, 0);
// }
// 
// 
// /* Symbol is used to represent any variable, function, or type name */
// typedef char * Symbol;
// 
// enum ValueType {
//     VTYPE_SYMBOL,
//     VTYPE_STRING,
//     VTYPE_INT,
//     VTYPE_FLOAT,
//     VTYPE_BOOL,
//     VTYPE_EXPR,
//     VTYPE_FUNCALL
// };
// 
// enum OperatorType {
//     OP_OR,
//     OP_AND,
//     OP_EQEQ,
//     OP_NOT_EQ,
//     OP_GREATER_EQ,
//     OP_GREATER,
//     OP_LESS_EQ,
//     OP_LESS,
//     OP_PLUS,
//     OP_MINUS,
//     OP_STAR,
//     OP_SLASH,
//     OP_UNARY_PLUS,
//     OP_UNARY_MINUS
// };
// 
// enum Type {
//     TYPE_INT,
//     TYPE_FLOAT,
//     TYPE_BOOL,
//     TYPE_STRING
// };
// 
// struct Expr;
// struct Value;
// 
// struct FunCall {
//     Symbol funname;
//     Values *values;
// };
// 
// struct Expr {
//     enum OperatorType op;
//     struct Value *val1;
//     struct Value *val2;
// };
// 
// struct Value {
//     enum ValueType type;
//     union {
//         char *string;
//         Symbol symbol;
//         long integer;
//         double floating;
//         long boolean;
//         struct Expr *expr;
//         struct FunCall *funcall;
//     };
// };
// 
// struct Statement;
// 
// struct Definition {
//     Symbol name;
//     enum Type type;
// };
// 
// struct Set {
//     Symbol symbol;
//     struct Value *val;
// };
// 
// struct IfStatement {
//     struct Value *condition;
//     Statements *if_stmts;
//     Statements *else_stmts;
// };
// 
// struct While {
//     struct Value *condition;
//     Statements *stmts;
// };
// 
// struct For {
//     struct Set *start;
//     struct Value *stop;
//     Statements *stmts;
// };
// 
// struct ForEach {
//     Symbol symbol;
//     struct Value *condition;
//     Statements *stmts;
// };
// 
// enum StatementType {
//     STMT_DIM,
//     STMT_SET,
//     STMT_IF,
//     STMT_WHILE,
//     STMT_FOR,
//     STMT_FOREACH,
//     STMT_FUNCTION_DEF,
//     STMT_EXIT_FOR,
//     STMT_EXIT_WHILE,
//     STMT_EXIT_FUNCTION,
//     STMT_FUNCALL
// };
// 
// struct Statement {
//     enum StatementType type;
//     union {
//         Dim *dim;
//         struct Set *set;
//         struct IfStatement *if_stmt;
//         struct While *while_stmt;
//         struct For *for_stmt;
//         struct ForEach *for_each;
//         struct FunCall *funcall;
//     };
// };
// 
