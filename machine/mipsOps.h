/* file "MIPSOps.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>
#ifndef MIPS_OPS_H
#define MIPS_OPS_H

const int OP_BASE_MIPS = 6000; /* start of ai_ops enumeration */

enum /* mi_ops */ {
    mo_FIRST_OP = OP_BASE_MIPS,        /* start of MIPS enumeration */

    #undef xx
    #define xx(opcode, string, format) opcode,
    #include "mips.data"

    mo_LAST_OP
} ;

enum /* mi_op_exts */ {
    moe_FIRST_OP_EXT = mo_LAST_OP,

    /** VAX/IEEE rounding mode qualifiers */
    moe_round_normal,           /* normal rounding mode controlled by FPCR */
    moe_round_chopped,          /* chopped, i.e. truncate */
    moe_round_p_inf,            /* round toward plus infinity */
    moe_round_m_inf,            /* round toward minus infinity */

    /** VAX/IEEE trap modes */
    moe_trap_none,              /* imprecise, underflow disabled, (and, for
                                 * IEEE, inexact disabled) */
    moe_trap_u,                 /* imprecise, underflow enabled, (and, for
                                 * IEEE, inexact disabled) */
    moe_trap_s,                 /* software and underflow disabled (not valid
                                 * option for IEEE FP) */
    moe_trap_su,                /* software, underflow enabled, (and, for
                                 * IEEE, inexact disabled) */
    moe_trap_sui,               /* software, underflow enabled, and inexact
                                 * enabled (not valid option for VAX) */

    /** VAX/IEEE convert-to-integer trap modes */
    moe_itrap_none,             /* imprecise, int overflow disabled, (and,
                                 * for IEEE, inexact disabled) */
    moe_itrap_v,                /* imprecise, int overflow enabled, (and,
                                 * for IEEE, inexact disabled) */
    moe_itrap_s,                /* software, int overflow disabled, (not
                                 * valid option for IEEE) */
    moe_itrap_sv,               /* software, int overflow enabled, (and,
                                 * for IEEE, inexact disabled) */
    moe_itrap_svi,              /* software, int overflow ensabled, and inexact
                                 * enabled (not valid option for VAX) */

    moe_LAST_OP_EXT
} ;

extern char *mips_op_string(mi_ops o);
extern mi_formats mips_which_mformat(mi_ops o);
extern char *mips_op_ext_string(mi_op_exts e);

extern int mips_invert_table[];

#endif /* MIPS_OPS_H */
