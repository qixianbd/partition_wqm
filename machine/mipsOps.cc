/* file "mipsOps.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*
 * Complete list of the mips assembly and pseudo- opcodes in string form.
 * Uses mips.data; modify that file to change strings or format.
 */

#include <suif1.h>
#include "machine_internal.h"

/*
 *  Opcode names
 */
static char *mips_op_names[] = {
    "mo_FIRST_OP",		/* start of mips opcode enumeration */

    #undef xx
    #define xx(opcode, string, format) string,
    #include "mips.data"

    "mo_LAST_OP"
} ;


/* mips_op_string() -- returns string name of an mi_ops value. */
char *mips_op_string(mi_ops o)
{
    if ((o < mo_FIRST_OP) || (o > mo_LAST_OP)) return "mo_ERROR";
    return mips_op_names[o - mo_FIRST_OP];
}


/*
 *  Opcode formats
 */
static int mips_op_formats[] = {
    mif_xx,	/* mo_FIRST_OP */

    #undef xx
    #define xx(opcode, string, format) format,
    #include "mips.data"

    mif_xx	/* mo_LAST_OP */
} ;


/* mips_which_mformat() -- returns the format of an mi_ops value. */
mi_formats mips_which_mformat(mi_ops o)
{
    assert_msg(((o >= mo_FIRST_OP) && (o <= mo_LAST_OP)),
        ("opcode value %d out of mips range",o));
    return mips_op_formats[o - mo_FIRST_OP];
}


/* Opcode extensions */
static char *mips_op_ext_names[] = {
    "moe_FIRST_OP_EXT",		/* start of mips opcode ext enum */

    /** VAX/IEEE rounding mode qualifiers */
    "",				/* normal rounding mode controlled by FPCR */
    "c",			/* chopped, i.e. truncate */
    "d",			/* round toward plus infinity */
    "m",			/* round toward minus infinity */

      /** VAX/IEEE trap modes */
    "",				/* imprecise, underflow disabled, (and, for
				 * IEEE, inexact disabled) */
    "u",			/* imprecise, underflow enabled, (and, for
				 * IEEE, inexact disabled) */
    "s",			/* software and underflow disabled (not valid
				 * option for IEEE FP) */
    "su",			/* software, underflow enabled, (and, for
				 * IEEE, inexact disabled) */
    "sui",			/* software, underflow enabled, and inexact
				 * enabled (not valid option for VAX) */

    /** VAX/IEEE convert-to-integer trap modes */
    "",				/* imprecise, int overflow disabled, (and,
				 * for IEEE, inexact disabled) */
    "v",			/* imprecise, int overflow enabled, (and,
				 * for IEEE, inexact disabled) */
    "s",			/* software, int overflow disabled, (not
				 * valid option for IEEE) */
    "sv",			/* software, int overflow enabled, (and,
				 * for IEEE, inexact disabled) */
    "svi",			/* software, int overflow ensabled, and inexact
				 * enabled (not valid option for VAX) */

    "moe_LAST_OP_EXT"
};


/* mips_op_ext_string() -- returns string name of a mi_op_exts value. */
char *mips_op_ext_string(mi_op_exts e)
{
    if ((e < moe_FIRST_OP_EXT) || (e > moe_LAST_OP_EXT)) return "moe_ERROR";
    return mips_op_ext_names[e - moe_FIRST_OP_EXT];
}
/* This table is used by the routine Invert_cbr_op(). */
int mips_invert_table[] = {
    mo_bc1f, 			mo_bc1t,
    mo_beq,			mo_bne,
    mo_bgez,			mo_bltz,
    mo_bgezal,			mo_bltzal,
    mo_bgtz,			mo_blez,
    mo_beqz,			mo_bnez,
    mo_bge,			mo_blt,
    mo_bgeu,			mo_bltu,
    mo_bgt,			mo_ble,
    mo_bgtu,			mo_bleu,
       
    // Ending case. DO NOT DELETE!!!
    -1,				-1
}; 



