/*  Top-level Dataflow Analysis Header File */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef DFA_H
#define DFA_H

/*
 *  Use a macro to include files so that they can be treated differently
 *  when compiling the library than when compiling an application.
 */

#ifdef DFALIB
#define DFAINCLFILE(F) #F
#else
#define DFAINCLFILE(F) <dfa/F>
#endif

#include DFAINCLFILE(bit_vector_dfa.h)
#include DFAINCLFILE(operand_bit_manager.h)
#include   DFAINCLFILE(live_var.h)
#include     DFAINCLFILE(live_var_more.h)  /* For scheduling. To be moved. */
#include   DFAINCLFILE(unset_var.h)
#include   DFAINCLFILE(def_teller.h)
#include     DFAINCLFILE(def_catalog.h)
#include       DFAINCLFILE(reaching_def.h)
#include DFAINCLFILE(bitset_function.h)
#include  DFAINCLFILE(bit_vector_dfa_plus.h)
#include  DFAINCLFILE(live_varAD.h)
#include  DFAINCLFILE(instr_catalog.h)

#endif /* DFA_H */
