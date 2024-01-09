#include "parser.h"
extern int yyparse(void);
int parse(void)
{
    return yyparse();
}

