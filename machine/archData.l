/* file "regInfo.l" -- flex file used to build a scanner for the
 * machsuif\impl\*.reg files. */

/*  Copyright (c) 1996-1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

%{
#include <suif_copyright.h>
#include <stdlib.h>
#include <string.h>
#include "archData.h"

extern int isatty(int);

//int yylineno = 1;

#undef yywrap		/* sometimes a macro by default */

%}


/* LEX definitions -- Parentheses are considered part of the assembly-
 * language string name for a register because the x86 ISA names their
 * FP stack registers as ST(i), where i is an integer between 0 and 7. */
V       [a-zA-Z_][-.a-zA-Z0-9]*
RNAME	[a-zA-Z_][-.a-zA-Z0-9()]*
I       -?[0-9]+


/* Used to literally transcribe register names between the "/" and 
 * the next register name description.  Without this context-sensitive
 * rule, we won't always get tSTRING tokens back with sval set correctly. 
 * This isn't just a numbering problem (getting tNUMBER instead of tSTRING
 * for registers named "13"), but also is a problem with the overloading
 * of tokens like tSP and the string "sp".  Without the context-sensitive
 * rule, we get a tSP token instead of a tSTRING with sval=="sp". */
%x sASM			


%%

<sASM>[\t ]+	;	/* Ignore whitespace */
[\t ]+		;	/* Ignore whitespace */

<sASM>#.*	;	/* Skip comments */
#.*		;	/* Skip comments */

<sASM>\n	{ BEGIN 0; yylineno++; }
\n		{ yylineno++; }

"VENDOR_OS"	return tVENDOR_OS;
"TOTAL_GRAINS"	return tTOTAL_GRAINS;
"BANK"		return tBANK;
"WIDTH"		return tWIDTH;
"GRAIN_SIZE"	return tGRAIN_SIZE;
"CONVENTION"	return tCONVENTION; 
"END_VOS"	return tEND_VOS;
"gpr"		return tGPR;
"fpr"		return tFPR;
"seg"		return tSEG;
"ctl"		return tCTL;
"const0"	return tCONST0;
"ra"		return tRA;
"sp"		return tSP;
"gp"		return tGP;
"fp"		return tFP;
"arg"		return tARG;
"ret"		return tRET;
"sav"		return tSAV;
"tmp"		return tTMP;
"asm_tmp"	return tASM_TMP; 
"gen"		return tGEN;
<sASM>","	{ BEGIN 0; return tCOMMA; } 
","		return tCOMMA;
"/"		{ BEGIN sASM; return tSLASH; }
<sASM>"-"	return tDASH;

<sASM>{RNAME}	{
		yylval.sval = strdup(yytext);
		return tSTRING;
		}

{V}		{
		yylval.sval = strdup(yytext);
		return tSTRING;
		}

<sASM>{I}	{
		/* return integers as strings since assembler names may
		 * look like integers, but we want to store them as strings */
		yylval.sval = strdup(yytext);
		return tSTRING;
		}

{I}		{
		yylval.ival = atoi(yytext);
		return tNUMBER;
		}

%%

/* The lexer calls yywrap to handle EOF conditions.  Since we read
 * only a single file at a time, we simply return 1, meaning no
 * more input after EOF. */
int yywrap()
{
    return 1;	/* no more input */
}
