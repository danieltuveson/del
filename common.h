#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

/* Global variable to hold ast of currently parsed ast
 * I would prefer to avoid globals, but I'm not sure how
 * to get Bison to return a value without one
 */
struct Ast ast;

#endif

