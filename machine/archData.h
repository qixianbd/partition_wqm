
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     tVENDOR_OS = 258,
     tEND_VOS = 259,
     tTOTAL_GRAINS = 260,
     tBANK = 261,
     tWIDTH = 262,
     tGRAIN_SIZE = 263,
     tCONVENTION = 264,
     tGPR = 265,
     tFPR = 266,
     tSEG = 267,
     tCTL = 268,
     tCONST0 = 269,
     tRA = 270,
     tSP = 271,
     tGP = 272,
     tFP = 273,
     tARG = 274,
     tRET = 275,
     tSAV = 276,
     tTMP = 277,
     tASM_TMP = 278,
     tGEN = 279,
     tSTRING = 280,
     tNUMBER = 281,
     tCOMMA = 282,
     tSLASH = 283,
     tDASH = 284
   };
#endif
/* Tokens.  */
#define tVENDOR_OS 258
#define tEND_VOS 259
#define tTOTAL_GRAINS 260
#define tBANK 261
#define tWIDTH 262
#define tGRAIN_SIZE 263
#define tCONVENTION 264
#define tGPR 265
#define tFPR 266
#define tSEG 267
#define tCTL 268
#define tCONST0 269
#define tRA 270
#define tSP 271
#define tGP 272
#define tFP 273
#define tARG 274
#define tRET 275
#define tSAV 276
#define tTMP 277
#define tASM_TMP 278
#define tGEN 279
#define tSTRING 280
#define tNUMBER 281
#define tCOMMA 282
#define tSLASH 283
#define tDASH 284




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 37 "archData.y"

    int ival;
    char *sval;



/* Line 1676 of yacc.c  */
#line 117 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


