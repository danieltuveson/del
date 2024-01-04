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
     ST_TO = 264,
     ST_IN = 265,
     ST_NEXT = 266,
     ST_WHILE = 267,
     ST_FUNCTION = 268,
     ST_EXIT = 269,
     ST_DIM = 270,
     ST_AS = 271,
     ST_STRING = 272,
     ST_INT = 273,
     ST_FLOAT = 274,
     ST_BOOL = 275,
     ST_AND = 276,
     ST_OR = 277,
     ST_NOT = 278,
     ST_NULL = 279,
     ST_PLUS = 280,
     ST_MINUS = 281,
     ST_STAR = 282,
     ST_SLASH = 283,
     ST_EQ = 284,
     ST_EQEQ = 285,
     ST_NOT_EQ = 286,
     ST_LESS_EQ = 287,
     ST_GREATER_EQ = 288,
     ST_LESS = 289,
     ST_GREATER = 290,
     ST_OPEN_PAREN = 291,
     ST_CLOSE_PAREN = 292,
     ST_COMMA = 293,
     ST_AMP = 294,
     ST_UNDERSCORE = 295,
     ST_SEMICOLON = 296,
     ST_NEWLINE = 297,
     UNARY_MINUS = 298,
     UNARY_PLUS = 299,
     T_STRING = 300,
     T_IDENTIFIER = 301,
     T_INT = 302,
     T_FLOAT = 303,
     T_OTHER = 304
   };
#endif
/* Tokens.  */
#define ST_IF 258
#define ST_THEN 259
#define ST_ELSEIF 260
#define ST_ELSE 261
#define ST_END 262
#define ST_FOR 263
#define ST_TO 264
#define ST_IN 265
#define ST_NEXT 266
#define ST_WHILE 267
#define ST_FUNCTION 268
#define ST_EXIT 269
#define ST_DIM 270
#define ST_AS 271
#define ST_STRING 272
#define ST_INT 273
#define ST_FLOAT 274
#define ST_BOOL 275
#define ST_AND 276
#define ST_OR 277
#define ST_NOT 278
#define ST_NULL 279
#define ST_PLUS 280
#define ST_MINUS 281
#define ST_STAR 282
#define ST_SLASH 283
#define ST_EQ 284
#define ST_EQEQ 285
#define ST_NOT_EQ 286
#define ST_LESS_EQ 287
#define ST_GREATER_EQ 288
#define ST_LESS 289
#define ST_GREATER 290
#define ST_OPEN_PAREN 291
#define ST_CLOSE_PAREN 292
#define ST_COMMA 293
#define ST_AMP 294
#define ST_UNDERSCORE 295
#define ST_SEMICOLON 296
#define ST_NEWLINE 297
#define UNARY_MINUS 298
#define UNARY_PLUS 299
#define T_STRING 300
#define T_IDENTIFIER 301
#define T_INT 302
#define T_FLOAT 303
#define T_OTHER 304




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 241 "test.y"
{
    char name[20];
    long integer;
    struct Expr *expr;
    struct Value *val;
    double floating;
}
/* Line 1529 of yacc.c.  */
#line 155 "test.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

