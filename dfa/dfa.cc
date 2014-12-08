/*  Dataflow Analysis Library Initialization and Finalization */

/*  Copyright (c) 1996-1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

/*
 *  Initialization and finalization functions for the DFA library.
 */

void
init_dfa (int & /* argc */, char * /* argv */ [])
{
    /* Names of unregistered annotations */
    k_definee_ids = lexicon->enter("definee_ids")->sp;
    k_def_point_1st = lexicon->enter("def_point_1st")->sp;
    k_def_point_num = lexicon->enter("def_point_num")->sp;
    ANNOTE(k_instr_catalog,"instr_catalog",FALSE);
}


void
exit_dfa ()
{
}
