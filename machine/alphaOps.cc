/* file "alphaOps.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*
 * Complete list of the ALPHA assembly and pseudo- opcodes in string form.
 * Uses alpha.data; modify that file to change strings or format.
 */

#include <suif1.h>
#include "machine_internal.h"

/*
 *  Opcode names
 */
static char *alpha_op_names[] = {
    "ao_FIRST_OP",		/* start of ALPHA opcode enumeration */

    #undef xx
    #define xx(opcode, string, format) string,
    #include "alpha.data"

    "ao_LAST_OP"
} ;


/* alpha_op_string() -- returns string name of an mi_ops value. */
char *alpha_op_string(mi_ops o)
{
    if ((o < ao_FIRST_OP) || (o > ao_LAST_OP)) return "ao_ERROR";
    return alpha_op_names[o - ao_FIRST_OP];
}


/*
 *  Opcode formats
 */
static int alpha_op_formats[] = {
    mif_xx,	/* ao_FIRST_OP */

    #undef xx
    #define xx(opcode, string, format) format,
    #include "alpha.data"

    mif_xx	/* ao_LAST_OP */
} ;


/* alpha_which_mformat() -- returns the format of an mi_ops value. */
mi_formats alpha_which_mformat(mi_ops o)
{
    assert_msg(((o >= ao_FIRST_OP) && (o <= ao_LAST_OP)),
        ("opcode value %d out of ALPHA range",o));
    return alpha_op_formats[o - ao_FIRST_OP];
}


/* Opcode extensions */
static char *alpha_op_ext_names[] = {
    "aoe_FIRST_OP_EXT",		/* start of ALPHA opcode ext enum */

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

    "aoe_LAST_OP_EXT"
};


/* alpha_op_ext_string() -- returns string name of a mi_op_exts value. */
char *alpha_op_ext_string(mi_op_exts e)
{
    if ((e < aoe_FIRST_OP_EXT) || (e > aoe_LAST_OP_EXT)) return "aoe_ERROR";
    return alpha_op_ext_names[e - aoe_FIRST_OP_EXT];
}


/* This table is used by the routine Invert_cbr_op(). */
int alpha_invert_table[] = {
    ao_beq,			ao_bne,
    ao_beq_N,			-1, 
    ao_beq_T,			-1, 
    ao_bge,			ao_blt,
    ao_bge_N,			-1, 
    ao_bge_T,			-1, 
    ao_bgt,			ao_ble,
    ao_bgt_N,			-1, 
    ao_bgt_T,			-1, 
    ao_blbc,			ao_blbs,
    ao_blbc_N,			-1, 
    ao_blbc_T,			-1, 
    ao_blbs_N,			-1, 
    ao_blbs_T,			-1, 
    ao_ble_N,			-1, 
    ao_ble_T,			-1, 
    ao_blt_N,			-1, 
    ao_blt_T,			-1, 
    ao_bne_N,			-1, 
    ao_bne_T,			-1, 
    ao_fbeq,			ao_fbne,
    ao_fblt,			ao_fbge,
    ao_fbgt,			ao_fble,			

    // Ending case. DO NOT DELETE!!!
    -1,				-1
}; 
