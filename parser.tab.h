/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ST_IF = 258,
     ST_THEN = 259,
     ST_ELSEIF = 260,
     ST_ELSE = 261,
     ST_END = 262,
     ST_FOR = 263,
     ST_EACH = 264,
     ST_TO = 265,
     ST_IN = 266,
     ST_NEXT = 267,
     ST_WHILE = 268,
     ST_FUNCTION = 269,
     ST_EXIT = 270,
     ST_DIM = 271,
     ST_AS = 272,
     ST_STRING = 273,
     ST_INT = 274,
     ST_FLOAT = 275,
     ST_BOOL = 276,
     ST_AND = 277,
     ST_OR = 278,
     ST_NOT = 279,
     ST_NULL = 280,
     ST_PLUS = 281,
     ST_MINUS = 282,
     ST_STAR = 283,
     ST_SLASH = 284,
     ST_EQ = 285,
     ST_EQEQ = 286,
     ST_NOT_EQ = 287,
     ST_LESS_EQ = 288,
     ST_GREATER_EQ = 289,
     ST_LESS = 290,
     ST_GREATER = 291,
     ST_OPEN_PAREN = 292,
     ST_CLOSE_PAREN = 293,
     ST_COMMA = 294,
     ST_AMP = 295,
     ST_UNDERSCORE = 296,
     ST_SEMICOLON = 297,
     ST_NEWLINE = 298,
     UNARY_MINUS = 299,
     UNARY_PLUS = 300,
     T_STRING = 301,
     T_SYMBOL = 302,
     T_INT = 303,
     T_FLOAT = 304,
     T_OTHER = 305
   };
#endif
/* Tokens.  */
#define ST_IF 258
#define ST_THEN 259
#define ST_ELSEIF 260
#define ST_ELSE 261
#define ST_END 262
#define ST_FOR 263
#define ST_EACH 264
#define ST_TO 265
#define ST_IN 266
#define ST_NEXT 267
#define ST_WHILE 268
#define ST_FUNCTION 269
#define ST_EXIT 270
#define ST_DIM 271
#define ST_AS 272
#define ST_STRING 273
#define ST_INT 274
#define ST_FLOAT 275
#define ST_BOOL 276
#define ST_AND 277
#define ST_OR 278
#define ST_NOT 279
#define ST_NULL 280
#define ST_PLUS 281
#define ST_MINUS 282
#define ST_STAR 283
#define ST_SLASH 284
#define ST_EQ 285
#define ST_EQEQ 286
#define ST_NOT_EQ 287
#define ST_LESS_EQ 288
#define ST_GREATER_EQ 289
#define ST_LESS 290
#define ST_GREATER 291
#define ST_OPEN_PAREN 292
#define ST_CLOSE_PAREN 293
#define ST_COMMA 294
#define ST_AMP 295
#define ST_UNDERSCORE 296
#define ST_SEMICOLON 297
#define ST_NEWLINE 298
#define UNARY_MINUS 299
#define UNARY_PLUS 300
#define T_STRING 301
#define T_SYMBOL 302
#define T_INT 303
#define T_FLOAT 304
#define T_OTHER 305




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 29 "parser.y"
{
    char *string;
    char *symbol;
    long integer;
    struct Expr *expr;
    struct Value *val;
    double floating;
    struct List *stmts;
    struct List *definitions;
    struct Definition *definition;
    struct Statement *stmt;
    enum Type type;
}
/* Line 1529 of yacc.c.  */
#line 163 "parser.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

