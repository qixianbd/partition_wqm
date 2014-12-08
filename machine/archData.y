%{

/* file "regInfo.y" -- bison file used to build a parser for the
 * machsuif\impl\*.reg files. */

/*  Copyright (c) 1996-1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <string.h>
#include "machine_internal.h"

extern void yyerror(const char *);
extern "C" {
extern int yylex();
extern int yylineno;
extern unsigned char yytext[];
}

boolean correct_desc = FALSE; 
int rgs_seen;		/* register grains seen */
int current_bank, current_conv; 
int regs_in_b, regs_in_bc; 
int index_bc; 
reginfo *yy_r;		/* yy_r = target_regs from reginfo constructor */

void yy_rpair(int, char *);

%}

%union {
    int ival;
    char *sval;
}

%{
extern "C" YYSTYPE yylval;
%}

%start descs

%token tVENDOR_OS tEND_VOS
%token tTOTAL_GRAINS 
%token tBANK tWIDTH tGRAIN_SIZE tCONVENTION 
%token tGPR tFPR tSEG tCTL 
%token tCONST0 tRA tSP tGP tFP tARG tRET tSAV tTMP tASM_TMP tGEN
%type <sval> numstr
%token <sval> tSTRING
%token <ival> tNUMBER
%token tCOMMA tSLASH tDASH 


%%

descs	:	desc
	|	descs desc
	;

desc	:	beg_vos voslist
        {
		rgs_seen = 0; 
		if (correct_desc) {
			/* start array allocation/zeroing */
			assert(!yy_r->desc); 
			yy_r->num_in_b = new int[LAST_REG_BANK];
			yy_r->num_in_bc = 
				new int[LAST_REG_BANK * LAST_REG_CONV];
			yy_r->start_of_b = new int[LAST_REG_BANK];
			yy_r->start_of_bc = 
				new int[LAST_REG_BANK * LAST_REG_CONV];
			yy_r->width_of_b = new int[LAST_REG_BANK];
			yy_r->gsize_of_b = new int[LAST_REG_BANK];
			for (int i = 0; i < LAST_REG_BANK; ++i) {
				yy_r->num_in_b[i] = 0; 
				yy_r->start_of_b[i] = -1;
				yy_r->width_of_b[i] = -1; 
				yy_r->gsize_of_b[i] = -1; 
				for (int j = 0; j < LAST_REG_CONV; ++j) {
					int k = i * LAST_REG_CONV + j; 
					yy_r->num_in_bc[k] = 0; 
					yy_r->start_of_bc[k] = -1;
				} 
			}
		} 
	} total banks tEND_VOS
	{
		if (correct_desc && rgs_seen != yy_r->n)
			warning_line(NULL,
			     ("Fewer reg grains seen than specified")); 
		if (correct_desc) {
		    yy_r->n = rgs_seen; 
		} else {
		    warning_line(NULL, "%s not found in *.reg",
					target_arch->vendor_os()); 
		}
	}
	;

beg_vos	:	tVENDOR_OS 
        {
	        correct_desc = FALSE;
	}
        ;

voslist	:	vos
        |	voslist vos
        ;

vos	:	tSTRING 
	{
		correct_desc |= !strcmp($1, target_arch->vendor_os()); 
	}
	;

numstr	:	tNUMBER 
	{
		$$ = (char *) malloc(20 * sizeof(char)); 
		sprintf($$, "%d", $1); 
	} 
	|	tSTRING 
	{
		$$ = strdup($1);
	}
	;

total	:	tTOTAL_GRAINS tNUMBER 
	{
		if (correct_desc) {
			/* finish array allocation/zeroing */
			yy_r->n = $2;
			yy_r->desc = new reg_desc[yy_r->n]; 
			yy_r->m2a_map = new int[yy_r->n]; 
			for (int i = 0; i < yy_r->n; i++) 
				yy_r->m2a_map[i] = -1; 
		}
	}
	;

banks	:	banks bank
	|
	;

bank	:	tBANK bname {
                              regs_in_b = 0;
			      if (correct_desc)
				  yy_r->start_of_b[current_bank] = rgs_seen;
                            } binfo 
	{	
		if (correct_desc) yy_r->num_in_b[current_bank] = regs_in_b; 
	}
	;

bname	:	tGPR	{ current_bank = GPR; }
	|	tFPR	{ current_bank = FPR; }
	|	tSEG	{ current_bank = SEG; }
	|	tCTL	{ current_bank = CTL; }
	;

binfo	:	bitem
	|	binfo bitem
	;

bitem	:	tWIDTH tNUMBER {
			if (correct_desc) yy_r->width_of_b[current_bank] = $2; 
		} 
	|	tGRAIN_SIZE tNUMBER {
			if (correct_desc) yy_r->gsize_of_b[current_bank] = $2; 
		} 
	|	conv
	;

conv	:	tCONVENTION cname {
		    index_bc = current_bank * LAST_REG_CONV + current_conv;
		    regs_in_bc = 0;
		    if (correct_desc) yy_r->start_of_bc[index_bc] = rgs_seen;
				  } rlist
	{
		if (correct_desc) yy_r->num_in_bc[index_bc] = regs_in_bc; 
	}
	;

cname	:	tCONST0		{ current_conv = CONST0; }
	|	tRA		{ current_conv = RA; }
	|	tSP		{ current_conv = SP; }
	|	tGP		{ current_conv = GP; }
	|	tFP		{ current_conv = FP; }
	|	tARG		{ current_conv = ARG; }
	|	tRET		{ current_conv = RET; }
	|	tSAV		{ current_conv = SAV; }
	|	tTMP		{ current_conv = TMP; }
	|	tASM_TMP	{ current_conv = ASM_TMP; }
	|	tGEN		{ current_conv = GEN; }
	;

rlist	:	rpair
	|	rlist tCOMMA rpair
	;

rpair	:	tNUMBER tSLASH numstr	{ yy_rpair($1, $3); }
	|	tNUMBER tSLASH tDASH	{ yy_rpair($1, NULL); }
	;

%%


/* yy_rpair() -- process register-encoding/assembler-string pair.  The
 * encoding == -1 and the assembler string is NULL if this register is
 * an unnamed entry used as a placeholder for register allocation and
 * dependence analysis. */
void
yy_rpair(int e, char *nm)
{
    ++regs_in_b; 
    ++regs_in_bc; 

    if (correct_desc) {
	if (rgs_seen >= yy_r->n) {
	    warning_line(NULL, ("doubling register grain space"));
	    yy_r->desc = (struct reg_desc *)realloc(yy_r->desc, 
			 2 * yy_r->n * sizeof(reg_desc)); 
	    assert(yy_r->desc); 
	    yy_r->m2a_map = (int *)realloc(yy_r->m2a_map,
			 2 * yy_r->n * sizeof(int)); 
	    assert(yy_r->m2a_map); 
	    for (int i = yy_r->n; i < (yy_r->n * 2); i++)
		yy_r->m2a_map[i] = -1;
	    yy_r->n *= 2; 
	} 

	yy_r->desc[rgs_seen].bank = current_bank; 
	yy_r->desc[rgs_seen].conv = current_conv; 
	yy_r->desc[rgs_seen].encoding = e; 
	yy_r->desc[rgs_seen].name = nm; 

	if (e >= 0) {
	    /* update m2a_map only if this is a named register */
	    int m_idx = yy_r->start_of_b[current_bank] + e;
	    assert_msg((yy_r->m2a_map[m_idx] == -1),
		("reginfo::parse() -- two named entries use same encoding"));
	    yy_r->m2a_map[m_idx] = rgs_seen;
	}

	++rgs_seen; 
    }
}


void
yyerror(const char *errmsg)
{
    static int yyerror_entered = 0;
    fprintf(stderr, "line #%d: %s at '%s'\n", yylineno, errmsg, yytext);
    assert_msg((yyerror_entered++ < 10), ("yyerror -- giving up"));
}

