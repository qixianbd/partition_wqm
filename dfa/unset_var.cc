/* Unset variable analysis implementation */

/*  Copyright (c) 1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

/* 
 *  Constructing an instance of the class invokes unsetness analysis.
 */

unset_var_problem::unset_var_problem(cfg *graph, operand_bit_manager *obm,
				     instruction_def_use_f du_fun) :
  bit_vector_problem(forward, graph)
{
    bit_man = obm;
    instr_def_use_fun = du_fun;
    instr_def = new bit_set(0, bit_man->num_bits());
    instr_use = new bit_set(0, bit_man->num_bits());	// ignored in this analyzer
    defs = unset_ins = unset_outs = NULL;
    solve();
}

unset_var_problem::~unset_var_problem() {
    Delete_bit_set_array(defs);
    Delete_bit_set_array(unset_ins);
    Delete_bit_set_array(unset_outs);
    delete instr_def;
    delete instr_use;
}

boolean
unset_var_problem::unset_in(cfg_node *n, int var_num)
{
    assert(unset_ins);
    return (*unset_ins)[n->number()]->contains(var_num);
}

boolean
unset_var_problem::unset_out(cfg_node *n, int var_num)
{
    assert(unset_outs);
    return (*unset_outs)[n->number()]->contains(var_num);
}

bit_set *
unset_var_problem::unset_in_set(cfg_node *n)
{
    assert(unset_ins);
    return (*unset_ins)[n->number()];
}

bit_set *
unset_var_problem::unset_out_set(cfg_node *n)
{
    assert(unset_outs);
    return (*unset_outs)[n->number()];
}

/* unset_var_problem::build_sets() -- (re)initialize the local bit_sets
 * to setup to (re)solve the dataflow problem. */
void
unset_var_problem::build_sets()
{
    Delete_bit_set_array(defs);
    Delete_bit_set_array(unset_ins);
    Delete_bit_set_array(unset_outs);

    /* Create the arrays mapping blocks to bit vectors.  Initialize
     * each bit vector to the universal set.  (Everything's undefined
     * till proven otherwise.)
     */
    defs = new bit_set_array();
    unset_ins = new bit_set_array();
    unset_outs = new bit_set_array();
    for (unsigned i = 0; i < the_cfg->num_nodes(); i++) {
	defs->extend(new bit_set(0, bit_man->num_bits()));
	bit_set *new_in  = new bit_set(0, bit_man->num_bits(), TRUE);
	bit_set *new_out = new bit_set(0, bit_man->num_bits(), TRUE);
	new_in->universal();
	new_out->universal();
	unset_ins->extend(new_in);
	unset_outs->extend(new_out);

	build_local_info(i);
    }
}


/* unset_var_problem::build_local_info() -- This routine calculates the
 * local def set for the cfg_node numbered n.
 *
 * Any relevant procedure parameters are noted as being initialized in
 * the entry node (node 0).  (Actually, if a parameter is passed in a
 * register, it's that register that is implicitly defined.  The parameter's
 * initialization from the register is expected to be explicit in the code.)
 */
void
unset_var_problem::build_local_info(int n)
{
    (*defs)[n]->clear();

    cfg_node *cnode = the_cfg->node(n);
    if (cnode->is_end() || cnode->is_label())
	return;		/* no defs */

    if (n == 0) {
	assert(cnode->is_begin());
	sym_node_list_iter param_iter(the_cfg->tproc()->proc_syms()->params());

	while (!param_iter.is_empty()) {
	    var_sym *p = (var_sym *)param_iter.step();
	    assert(p->is_var());

	    operand opnd(p);
	    if (p->peek_annote(k_vsym_info) && vsym_passed_in_preg(p))
		opnd = operand(vsym_get_preg(p), p->type());

	    int index;
	    if (bit_man->lookup(opnd, &index))
		(*defs)[0]->add(index);
	}
	return;
    }

    /* sanity checks */
    assert_msg(!cnode->is_test(), ("unset_var_problem::build_local_info() -- "
				   "can't deal with CFG_TEST blocks"));
    assert(cnode->is_instr() || cnode->is_block());

    cfg_node_instr_iter cni_iter(cnode);
    while (!cni_iter.is_empty()) {
	tree_instr *ti = cni_iter.step();
	instruction *in = ti->instr();

	/* update def set to reflect this instruction */
	note_instr_def(n, in, instr_def);
    }
}

void
unset_var_problem::note_instr_def(int n, instruction *in, bit_set *def)
{
    def->clear();

    instr_def_use_fun(in, bit_man, def, instr_use);	// last arg ignored here
    
    *(*defs)[n] += *def;
}

/* unset_var_problem::solver_ops() -- propagates the unset_var
 * dataflow information through one block.  Returns TRUE if the
 * unset_in set for this node n changed. */

boolean
unset_var_problem::solver_ops(cfg_node *n)
{
    int nnum = n->number();
    bit_set *in = (*unset_ins)[nnum];
    bit_set *out = (*unset_outs)[nnum];

    /* in[n] = intersection_n(out[preds(n)]) */
    in->clear();

    cfg_node_list_iter p_iter(n->preds());
    while (!p_iter.is_empty())
	*in *= *((*unset_outs)[p_iter.step()->number()]);

    /* calculate out[n] = in[n] \ def[n] */
    bit_set t;             /* Separate declaration from assignment ... */
    t = *in;		   /* ... till bit_set gets a copy constructor */
    t -= *((*defs)[nnum]);

    /* update out set if necessary */
    if (t != *out) {
	*out = t;
	return TRUE;
    }
    return FALSE;
}
