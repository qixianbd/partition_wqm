/* More than live variable analysis */
/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

void
live_var_problem_more::dump(FILE *fp)
{
    unsigned int i;

    for(i=0; i<the_cfg->num_nodes(); i++) {
	fprintf(fp, "\nBlock_%d", the_cfg->node(i)->number()); 
	fprintf(fp, "\ndef:"); (*defs)[i]->print(fp);
	fprintf(fp, "\nuse:"); (*uses)[i]->print(fp);
	fprintf(fp, "\nall_def:"); (*all_defs)[i]->print(fp);
	fprintf(fp, "\nlive_in:"); (*live_ins)[i]->print(fp);
	fprintf(fp, "\nlive_out:"); (*live_outs)[i]->print(fp);
    }
}

live_var_problem_more::live_var_problem_more(cfg *graph, operand_bit_manager *opd_bit_man, instruction_def_use_f du_fun) : live_var_problem(graph, opd_bit_man, du_fun)
{
    all_defs = NULL;
    solve();
}

void
live_var_problem_more::build_sets()
{
    Delete_bit_set_array(all_defs);
    all_defs = new bit_set_array();

    for (int i = the_cfg->num_nodes(); i > 0; i--)
	all_defs->extend(new bit_set(0, bit_man->num_bits()));

    live_var_problem::build_sets();
}

void
live_var_problem_more::build_local_info(int n)
{
    (*all_defs)[n]->clear();
    live_var_problem::build_local_info(n);
}

/*
 * This is a duplication of live_var_problem::note_instr_def_use
 * then add the computation for all-def set
 */
void
live_var_problem_more::note_instr_def_use(int n, instruction *in,
					  bit_set *def, bit_set *use)
{
    def->clear();
    use->clear();			     

    instr_def_use_fun(in, bit_man, def, use);
    
    bit_set *&d = (*defs)[n];
    bit_set *&u = (*uses)[n];
    bit_set *&alld = (*all_defs)[n];

    /* Exclude new uses that have earlier defs.  Exclude new defs
     * that have earlier uses, including those in this instruction. */
    *alld += *def;
    *use -= *d;
    *u += *use;
    *def -= *u;
    *d += *def;
}

void 
live_var_problem_more::local_info_update( cfg_node *cn )
{
    int n = cn->number();
    assert( (*defs)[n] );
    assert( (*uses)[n] );
    assert( (*all_defs)[n] );
    build_local_info( n );
}

void
live_var_problem_more::add_block_update( cfg_node *cn )
{
    int n = cn->number();
    /*
     * cn is the new block, if old array has x elements,
     * n must be x+1
     */
    assert( all_defs->ub()==n );

    defs->extend(new bit_set(0,bit_man->num_bits()));
    uses->extend(new bit_set(0,bit_man->num_bits()));
    live_ins->extend(new bit_set(0,bit_man->num_bits()));
    live_outs->extend(new bit_set(0,bit_man->num_bits()));
    all_defs->extend(new bit_set(0,bit_man->num_bits()));
    build_local_info(n);

    solve();
}

/*
 * liveness update on a subset of cfg nodes, not entire cfg
 */
void
live_var_problem_more::subset_liveness_update( cfg_node_list *cnl )
{
    /* iterate until no changes in IN set */
    boolean changes = TRUE;
    while (changes) {
	changes = FALSE;
	cfg_node_list_iter cnli(cnl);
	while (!cnli.is_empty()) {
	    cfg_node *cn = cnli.step();
	    changes = solver_ops(cn);
	}
    }

}

/*
 * count the operands being defined in a subset of cfg node 
 */
void
live_var_problem_more::subset_all_def( cfg_node_list *cnl, bit_set *alld)
{
    bit_set res(0, bit_man->num_bits());

    cfg_node_list_iter cnli(cnl);
    while (!cnli.is_empty()) {
	cfg_node *cn = cnli.step();
	res += *((*all_defs)[cn->number()]);
    }	
    alld->copy(&res);
}

/*
 * count the operands being used in a subset of cfg node
 */
void
live_var_problem_more::subset_all_use( cfg_node_list *cnl, bit_set *allu)
{
    bit_set res(0, bit_man->num_bits());

    cfg_node_list_iter cnli(cnl);
    while (!cnli.is_empty()) {
	cfg_node *cn = cnli.step();
	res += *((*uses)[cn->number()]);
    }	
    allu->copy(&res);
}

/*
 * count the operands which are defined more than once in a subset
 * of cfg nodes
 */
void
live_var_problem_more::subset_repeated_def( cfg_node_list *cnl, bit_set *rdef)
{
    bit_set seen_def(0, bit_man->num_bits());
    bit_set repeat_def(0, bit_man->num_bits());
    bit_set def(0, bit_man->num_bits());
    bit_set tmp(0, bit_man->num_bits());

    cfg_node_list_iter cnli(cnl);
    while (!cnli.is_empty()) {
	cfg_node *cn = cnli.step();
	if (!(cn->is_block() || cn->is_instr()))
	    continue;
	cfg_node_instr_iter cni_iter(cn);
	while (!cni_iter.is_empty()) {
	    tree_instr *ti = cni_iter.step();
	    note_instr_def_use(cn->number(), ti->instr(), &def, &tmp);
	    tmp.set_intersect(&seen_def, &def);
	    repeat_def += tmp;
	    seen_def += def;
	}
    }	
    rdef->copy(&repeat_def);
}

