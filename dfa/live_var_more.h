/* More than live-variable analysis */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef LIVE_VAR_MORE_H
#define LIVE_VAR_MORE_H

/*
 * Derive from live_var problem.  Add all_defs, subgraph live_in and
 * subgraph def_all computation, and maybe more in the future.
 */
class live_var_problem_more : public live_var_problem {
private:
    bit_set_array *all_defs;
    void build_sets();
    void build_local_info(int node);
    void note_instr_def_use(int, instruction *, bit_set *, bit_set *);
public:
    live_var_problem_more(cfg *graph, operand_bit_manager *opd_bit_man, instruction_def_use_f du_fun);
    ~live_var_problem_more()
    { Delete_bit_set_array(all_defs); } 
    bit_set *all_def_set(cfg_node *cn) 
    { return (*all_defs)[cn->number()]; }
    bit_set *use_set(cfg_node *cn) 
    { return (*uses)[cn->number()]; }
    void local_info_update(cfg_node *cn);
    boolean single_block_update(cfg_node *cn)
    { return solver_ops(cn); }
    void add_block_update(cfg_node *cn);
    void subset_all_def(cfg_node_list *cnl, bit_set *alld);
    void subset_all_use(cfg_node_list *cnl, bit_set *allu);
    void subset_repeated_def(cfg_node_list *cnl, bit_set *rdef);
    void subset_liveness_update(cfg_node_list *cnl);
    void dump(FILE *fp=stdout);
};

#endif /* LIVE_VAR_MORE_H */
