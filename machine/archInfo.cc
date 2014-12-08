/* file "archInfo.cc */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/* The following class captures target-specific information for
 * machsuif routines.  A default constructor is available so that
 * low-suif pass can invoke machsuif helper routines that work
 * with both low-suif and machsuif instruction lists.
 */


#include <suif1.h>
#include <string.h>
#include "machine_internal.h"

#ifndef WIN32
//extern "C" char *getenv(const char *);
//extern "C" char *strcat(char *, const char *);
#endif

extern int yyparse();

/* 
 * Global variable for current target architecture
 */
archinfo *target_arch = NULL; 
reginfo *target_regs = NULL;

/*
 * Static variables
 */
static char *reg_bank_names[] = {
    "GPR",
    "FPR",
    "SEG",
    "CTL"
};
static char *reg_conv_names[] = {
    "CONST0",
    "RA",
    "SP",
    "GP",
    "FP",
    "ARG",
    "RET",
    "SAV",
    "TMP",
    "ASM_TMP",
    "GEN"
};


/* archInfo::fopen_mdfile() -- Find the file that best matches the current
 * architecture, version, and implementation in the database of 
 * machine-specific description files.  Look for the one that has
 * the specified file extension.  Return the most specific file
 * name found. */
FILE *
archinfo::fopen_mdfile (char *ext, char *mode)
{
    /* Check to see if user specified a different directory of
     * description files via the MACHSUIF_IMPL_DIR environment variable.
     * If yes, use it.  Otherwise, create the full pathname for 
     * the default architecture description file named
     * $SUIFHOME/include/machine/<<fname>> */
    char *dir_name = getenv("MACHSUIF_IMPL_DIR");
    if (!dir_name) {
	char *suifhome = getenv("SUIFHOME");
	char *path = "/include/impl/"; 
	assert_msg(suifhome, ("reginfo::reginfo() -- $SUIFHOME undefined"));
	dir_name = new char[strlen(suifhome) + strlen(path) + 1];
	strcpy(dir_name, suifhome); 
	strcat(dir_name, path); 
    }
    assert(dir_name); 
    
    /* Search from most to least specific file names 
     * If we get an errno that's not equal to ENOENT, then something
     * else broke, so return NULL to the caller and let them handle
     * the error condition */
    char *file_name = new char[strlen(dir_name) + strlen(fam) + strlen(ver)
	+ strlen(impl) + strlen(ext) + 20]; 

    sprintf(file_name, "%s%s-%s-%s.%s", dir_name, fam, ver, impl, ext); 
    FILE *fd = fopen(file_name, mode); 
    if (fd || errno != ENOENT) 
	return fd; 
    sprintf(file_name, "%s%s-%s.%s", dir_name, fam, ver, ext); 
    fd = fopen(file_name, mode); 
    if (fd || errno != ENOENT)
	return fd; 
    sprintf(file_name, "%s%s.%s", dir_name, fam, ext); 
    fd = fopen(file_name, mode); 
    if (fd || errno != ENOENT)
	return fd; 

    assert_msg(NULL, ("archinfo::fopen_mdfile() -- cannot find arch file %s", 
	file_name)); 
    return NULL; 
}

/* archinfo::print() -- summarize target architecture information
 * in human-readable format. */
void
archinfo::print(FILE *fd)
{
    fprintf(fd, "ARCHinfo:\tfamily = \t\t%s\n", family());
    fprintf(fd, "\t\tversion = \t\t%s\n", version());
    fprintf(fd, "\t\tvendor_os = \t\t%s\n", vendor_os());
    fprintf(fd, "\t\timplementation = \t%s\n", implementation());
    fprintf(fd, "\n");
}


extern "C" FILE *yyin; 
reginfo::reginfo(FILE *fd)
{
    /* Need to create valid pointer for parsing and data structure
     * initialization because target_regs is not valid until after
     * this constructor completes. */
    extern reginfo *yy_r;
    yy_r = this;

    /* set default values for private variables */
    n = -1; 
    num_in_b = NULL; 
    num_in_bc = NULL; 
    start_of_b = NULL;
    start_of_bc = NULL;
    width_of_b = NULL; 
    gsize_of_b = NULL;
    desc = NULL; 
    m2a_map = NULL;

    /* parse the description file and fill in the data structures */
    yyin = fd; 
    while (!feof(yyin))
	yyparse(); 
    fclose(fd); 

    /* sanity check -- should have some non-negative number of hard regs */
    assert_msg((n >= 0), ("reginfo::reginfo -- didn't parse mdfile correctly"));
}


reginfo::~reginfo()
{
    if (num_in_b) delete[] num_in_b;
    if (num_in_bc) delete[] num_in_bc;
    if (start_of_b) delete[] start_of_b;
    if (start_of_bc) delete[] start_of_bc;
    if (width_of_b) delete[] width_of_b;
    if (gsize_of_b) delete[] gsize_of_b;
    if (desc) delete[] desc;
    if (m2a_map) delete[] m2a_map;
}


/* reginfo::a2m() -- map the abstract register number to the real machine
 * encoding for this register.  Remember that this encoding is relative
 * to the bank containing the abstract register.  Abstract registers that
 * are placeholders (i.e. that don't correspond to any nameable architecture
 * register) return -1. */
int
reginfo::a2m(int ar)
{
    int e = describe(ar).encoding;
    if (e < 0) 
	warning_line(NULL,
	    "Requested encoding for unnamed abstract reg %d", ar);
    return e;
}

/* reginfo::m2a() -- map the real machine encoding for this register to its
 * abstract register number.  Remember that this encoding is relative
 * to the bank containing the register and that the mr value is based
 * in units of register grains for this bank.  If the mapping returns
 * a negative value, then the register grain does not correspond to a
 * nameable register in the architecture. */
int
reginfo::m2a(int b, int mr)
{
    assert((mr < num_in_b[b]) && (mr >= 0));
    int ar = m2a_map[start_of(b) + mr];
    if (ar < 0) 
	warning_line(NULL,
	    "Requested abstract reg name for unnameable reg grain %d", ar);
    return ar;
}


/* reginfo::print() -- print out the register information for this
 * architecture in human-readable format. */
void
reginfo::print(FILE *fd)
{
    fprintf(fd, "REGinfo: %d register grains total\n", n); 
    for (int b = 0; b < LAST_REG_BANK; ++b) {
	if (!num_in(b))
	    continue; 
	fprintf(fd, "\n    Bank %s (%d bits wide with %d-bit grains):\n",
		reg_bank_string(b), width_of(b), grain_size_of(b)); 
	fprintf(fd, "\t(convention, num_in): abstract_num="
		"machine_encoding/assembler_name\n");
	for (int c = 0; c < LAST_REG_CONV; ++c) {
	    if (!num_in(b, c))
		continue;
	    fprintf(fd, "\t(%s, %d):\t", reg_conv_string(c), num_in(b, c)); 
	    for (int i = 0; i < num_in(b, c); ++i) {
	        int ar = lookup(b, c, i);
		reg_desc ard = describe(ar);
		if (i && ((i % 5) == 0)) fprintf(fd, "\n\t\t\t");
		fprintf(fd, "%d=%d/", ar, ard.encoding);
		if (ard.name) fprintf(fd,"%s ", ard.name); 
		else fprintf(fd,"- ");
	    }
	    fprintf(fd, "\n"); 
	}
    }
}


/* reg_bank_string() -- returns the string name of the specified
 * register bank. */
char *
reg_bank_string(int b)
{
    if ((b < 0) || (b >= LAST_REG_BANK)) return "rb_ERROR";
    return reg_bank_names[b];
}

/* reg_conv_string() -- returns the string name of the specified
 * register convention. */
char *
reg_conv_string(int c)
{
    if ((c < 0) || (c >= LAST_REG_CONV)) return "rc_ERROR";
    return reg_conv_names[c];
}

