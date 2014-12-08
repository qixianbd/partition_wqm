/* file "machineDefs.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef MACHINE_DEFS_H
#define MACHINE_DEFS_H

/**** Global definitions and declarations for machine constants ****/
const int SIZE_OF_BYTE = 8;             /* everything defined in bits */
const int SIZE_OF_HALFWORD = 16;
const int SIZE_OF_WORD = 32;
const int SIZE_OF_SINGLE = 32;
const int SIZE_OF_DOUBLE = 64;
const int SIZE_OF_QUAD = 128;

const int SHIFT_BITS_TO_BYTES = 3;
const int SHIFT_BYTES_TO_WORDS = 2;
const int DWORD_ALIGNMENT_MASK = 15;  /* off 16-byte boundary? */
const int WORD_ALIGNMENT_MASK = 7;    /* off 8-byte boundary? */
const int BYTE_MASK = 3;              /* off 4-byte boundary? */

const int BYTES_IN_WORD = SIZE_OF_WORD >> SHIFT_BITS_TO_BYTES;
const int BYTES_IN_DOUBLE = SIZE_OF_DOUBLE >> SHIFT_BITS_TO_BYTES;
const int BYTES_IN_QUAD = SIZE_OF_QUAD >> SHIFT_BITS_TO_BYTES;

/**** Opcode definitions ****/
typedef int mi_ops;
typedef int mi_op_exts;

const int io_null = -1;         /* null opcode extension to SUIF op space */

/* Definitions for the SUIF instruction set architecture. */
const int OP_BASE_SUIF = 0;     /* start of if_ops enumeration */

/**** Format tags ****/
typedef int mi_formats;
extern int which_mformat(mi_ops o);

enum /* mi_formats */ { /* machine instruction formats */
    mif_xx,             /* pseudo-op and other free formats */
    mif_lab,            /* labels */
    mif_bj,             /* branch/jump to label or through register */
    mif_rr,             /* general format */
    mif_LAST_FMT
};

#endif
