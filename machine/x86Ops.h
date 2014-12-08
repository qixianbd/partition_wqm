/* file "x86Ops.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1996-1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*
 * Complete listing of the x86 assembly opcodes, pseudo-ops, and
 * opcode extensions, and the code generator/schedulers pseudo-ops.
 *
 * The special values xo_FIRST_OP and xo_LAST_OP must bracket the
 * entire set of x86 opcodes since they are used in range checks.
 */

#ifndef X86_OPS_H
#define X86_OPS_H

const int OP_BASE_X86 = 4000;	/* start of xi_ops enumeration */

enum /* mi_ops */ {
    xo_FIRST_OP = OP_BASE_X86,	/* start of X86 enumeration */

    #undef xx
    #define xx(opcode, string, format) opcode,
    #include "x86.data"

    xo_LAST_OP
} ;


enum /* mi_op_exts */ {			/* instruction prefixes */
    xoe_FIRST_OP_EXT = xo_LAST_OP,

    xoe_lock,				/* ensure exclusive access
					   ... assert lock# signal */

    xoe_rep,				/* repeat string operation */
    xoe_repe,				/* ... if equal */
    xoe_repne,				/* ... if not equal */
    xoe_repnz,				/* ... if not zero */
    xoe_repz,				/* ... if zero */

    xoe_LAST_OP_EXT
} ;


/*
 *  The x86_op_string function returns the string name of an
 *  x86 opcode value. The x86_which_mformat function returns the
 *  machine instruction format used by each x86 opcode.  The
 *  x86_op_ext_string function returns the string name of the
 *  x86 opcode extension value.
 */
extern char *x86_op_string(mi_ops o);
extern mi_formats x86_which_mformat(mi_ops o);
extern char *x86_op_ext_string(mi_op_exts e);


/* Helper tables */
extern int x86_invert_table[];

#endif
