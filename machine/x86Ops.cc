/* file "x86Ops.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1996-1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*
 * Complete list of the x86 assembly and pseudo- opcodes in string form.
 */

#include <suif1.h>
#include "machine_internal.h"

/*
 *  Opcode names
 */
static char *x86_op_names[] = {
    "xo_FIRST_OP",		/* start of x86 opcode enumeration */

    #undef xx
    #define xx(opcode, string, format) string,
    #include "x86.data"

    "xo_LAST_OP"
} ;


/* x86_op_string() -- returns string name of an mi_ops value. */
char *x86_op_string(mi_ops o)
{
    if ((o < xo_FIRST_OP) || (o > xo_LAST_OP)) return "xo_ERROR";
    return x86_op_names[o - xo_FIRST_OP];
}


/*
 *  Opcode formats
 */
static int x86_op_formats[] = {
    mif_xx,	/* xo_FIRST_OP */

    #undef xx
    #define xx(opcode, string, format) format,
    #include "x86.data"

    mif_xx	/* xo_LAST_OP */
} ;


/* x86_which_mformat() -- returns the format of an mi_ops value. */
mi_formats x86_which_mformat(mi_ops o)
{
    assert_msg(((o >= xo_FIRST_OP) && (o <= xo_LAST_OP)),
        ("opcode value %d out of X86 range",o));
    return x86_op_formats[o - xo_FIRST_OP];
}


/* Opcode extensions */
static char *x86_op_ext_names[] = {
    "xoe_FIRST_OP_EXT",		/* start of X86 opcode ext enum */

    "lock",				/* ensure exclusive access
					   ... assert lock# signal */

    "rep",				/* repeat string operation */
    "repe",				/* ... if equal */
    "repne",				/* ... if not equal */
    "repnz",				/* ... if not zero */
    "repz",				/* ... if zero */

    "xoe_LAST_OP_EXT"
};


/* x86_op_ext_string() -- returns string name of a mi_op_exts value. */
char *x86_op_ext_string(mi_op_exts e)
{
    if ((e < xoe_FIRST_OP_EXT) || (e > xoe_LAST_OP_EXT)) return "xoe_ERROR";
    return x86_op_ext_names[e - xoe_FIRST_OP_EXT];
}


/* This table is used by the routine Invert_cbr_op(). */
int x86_invert_table[] = {
    xo_ja,			xo_jna,
    xo_jae,			xo_jnae,
    xo_jb,			xo_jnb,
    xo_jbe,			xo_jnbe,
    xo_jc,			xo_jnc,
    xo_je,			xo_jne,
    xo_jg,			xo_jng,
    xo_jge,			xo_jnge,
    xo_jl,			xo_jnl,
    xo_jle,			xo_jnle,
    xo_jo,			xo_jno,
    xo_jp,			xo_jnp,
    xo_jpe,			xo_jpo,
    xo_js,			xo_jns,
    xo_jz,			xo_jnz,
    
    xo_loop,			xo_jecxz,
    xo_loope,			xo_loopne,
    xo_loopz,			xo_loopnz,

    // Ending case. DO NOT DELETE!!!
    -1,				-1
}; 

