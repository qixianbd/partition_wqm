/* Implementation of bit-vector-based dataflow analysis problems */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

/* 
 *  Build a new instance of a bit-vector DFA problem.
 */
bit_vector_problem::bit_vector_problem(DFA_direction_type dir, cfg *graph)
{
    dfa_dir = dir;
    the_cfg = graph;
}


/* bit_vector_problem::solve() -- Iteratively traverse the cfg in the
 * appropriate order using the meet operation and transfer functions
 * (accessible via solver_ops()) in order to reach a fixed-point solution
 * to the problem.  If a fixed-point solution is not found within the
 * iteration_limit, return FALSE. */
boolean
bit_vector_problem::solve(int iteration_limit)
{
    /* construct the required bit sets for iteration (virtual function) */
    build_sets();

    int iteration = 0;
    boolean changes = TRUE;

    /* initialize traversal list in depth-first order for forward
     * problems, and reverse dfo for backward ones (dragon
     * book [ASU], section 10.10) */
    boolean fwd = (direction() == forward);
    cfg_node_list *df_list = the_cfg->reverse_postorder_list(fwd);

    /* iterate till fixed-point or till exceed iteration_limit */
    while (changes && (iteration < iteration_limit)) {
	iteration++;
	changes = FALSE;

	cfg_node_list_iter df_list_iter(df_list);
	while (!df_list_iter.is_empty()) {
	    cfg_node *n = df_list_iter.step(); 
	    if (solver_ops(n)) changes = TRUE;
	}
    }

    /* did we reach a fixed-point solution? */
    return changes;
}
