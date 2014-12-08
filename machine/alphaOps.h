/* file "alphaOps.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef ALPHA_OPS_H
#define ALPHA_OPS_H

const int OP_BASE_ALPHA = 2000; /* start of ai_ops enumeration */

enum /* mi_ops */ {
    ao_FIRST_OP = OP_BASE_ALPHA,        /* start of ALPHA enumeration */

    #undef xx
    #define xx(opcode, string, format) opcode,
    #include "alpha.data"

    ao_LAST_OP
} ;

enum /* mi_op_exts */ {
    aoe_FIRST_OP_EXT = ao_LAST_OP,

    /** VAX/IEEE rounding mode qualifiers */
    aoe_round_normal,           /* normal rounding mode controlled by FPCR */
    aoe_round_chopped,          /* chopped, i.e. truncate */
    aoe_round_p_inf,            /* round toward plus infinity */
    aoe_round_m_inf,            /* round toward minus infinity */

    /** VAX/IEEE trap modes */
    aoe_trap_none,              /* imprecise, underflow disabled, (and, for
                                 * IEEE, inexact disabled) */
    aoe_trap_u,                 /* imprecise, underflow enabled, (and, for
                                 * IEEE, inexact disabled) */
    aoe_trap_s,                 /* software and underflow disabled (not valid
                                 * option for IEEE FP) */
    aoe_trap_su,                /* software, underflow enabled, (and, for
                                 * IEEE, inexact disabled) */
    aoe_trap_sui,               /* software, underflow enabled, and inexact
                                 * enabled (not valid option for VAX) */

    /** VAX/IEEE convert-to-integer trap modes */
    aoe_itrap_none,             /* imprecise, int overflow disabled, (and,
                                 * for IEEE, inexact disabled) */
    aoe_itrap_v,                /* imprecise, int overflow enabled, (and,
                                 * for IEEE, inexact disabled) */
    aoe_itrap_s,                /* software, int overflow disabled, (not
                                 * valid option for IEEE) */
    aoe_itrap_sv,               /* software, int overflow enabled, (and,
                                 * for IEEE, inexact disabled) */
    aoe_itrap_svi,              /* software, int overflow ensabled, and inexact
                                 * enabled (not valid option for VAX) */

    aoe_LAST_OP_EXT
} ;

extern char *alpha_op_string(mi_ops o);
extern mi_formats alpha_which_mformat(mi_ops o);
extern char *alpha_op_ext_string(mi_op_exts e);

extern int alpha_invert_table[];

#endif /* ALPHA_OPS_H */
