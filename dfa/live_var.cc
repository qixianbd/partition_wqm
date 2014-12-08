/* Live-variable analysis implementation */

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
 *  Constructing an instance of the class invokes liveness analysis.
 */

live_var_problem::live_var_problem(cfg *graph, operand_bit_manager *obm,
				   instruction_def_use_f du_fun) :
  bit_vector_problem(backward, graph)
{
    bit_man = obm;
    instr_def_use_fun = du_fun;
    instr_def = new bit_set(0, bit_man->num_bits());
    instr_use = new bit_set(0, bit_man->num_bits());
    defs = uses = live_ins = live_outs = NULL;
    solve();
}

live_var_problem::~live_var_problem() {
    Delete_bit_set_array(defs);
    Delete_bit_set_array(uses);
    Delete_bit_set_array(live_ins);
    Delete_bit_set_array(live_outs);
    delete instr_def;
    delete instr_use;
}

boolean
live_var_problem::live_in(cfg_node *n, int var_num)
{
    assert(live_ins);
    return (*live_ins)[n->number()]->contains(var_num);
}

boolean
live_var_problem::live_out(cfg_node *n, int var_num)
{
    assert(live_outs);
    return (*live_outs)[n->number()]->contains(var_num);
}

bit_set *
live_var_problem::live_in_set(cfg_node *n)
{
    assert(live_ins);
    return (*live_ins)[n->number()];
}

bit_set *
live_var_problem::live_out_set(cfg_node *n)
{
    assert(live_outs);
    return (*live_outs)[n->number()];
}

/* live_var_problem::build_sets() -- (re)initialize the local bit_sets
 * to setup to (re)solve the dataflow problem. */
void
live_var_problem::build_sets()
{
    Delete_bit_set_array(defs);
    Delete_bit_set_array(uses);
    Delete_bit_set_array(live_ins);
    Delete_bit_set_array(live_outs);

    /* create and initialize the bit_set arrays in each cfg block */
    defs = new bit_set_array();
    uses = new bit_set_array();
    live_ins = new bit_set_array();
    live_outs = new bit_set_array();
    for (unsigned i = 0; i < the_cfg->num_nodes(); i++) {
	defs->extend(new bit_set(0, bit_man->num_bits()));
	uses->extend(new bit_set(0, bit_man->num_bits()));
	live_ins->extend(new bit_set(0, bit_man->num_bits()));
	live_outs->extend(new bit_set(0, bit_man->num_bits()));

	build_local_info(i);
    }
}


/* live_var_problem::build_local_info() -- This routine calculates the
 * local def and use sets for the cfg_node numbered n.
 * Node 0, the entry node, is treated as defining each procedure
 * parameter. */
void
live_var_problem::build_local_info(int n)
{
    (*defs)[n]->clear();
    (*uses)[n]->clear();

    cfg_node *cnode = the_cfg->node(n);
    if (cnode->is_end() || cnode->is_label())
	return;		/* no defs or uses */

    if (n == 0) {
	assert(cnode->is_begin());
	sym_node_list_iter param_iter(the_cfg->tproc()->proc_syms()->params());

	while (!param_iter.is_empty()) {
	    var_sym *p = (var_sym *)param_iter.step();
	    assert(p->is_var());
	    int index;
	    if (bit_man->lookup(operand(p), &index))
		(*defs)[0]->add(index);
	}
	return;
    }
    /* sanity checks */
    assert_msg(!cnode->is_test(), ("live_var_problem::build_local_info() -- "
				   "can't deal with CFG_TEST blocks"));
    assert(cnode->is_instr() || cnode->is_block());

    /* walk through instruction list */
    cfg_node_instr_iter cni_iter(cnode);
    while (!cni_iter.is_empty()) {
	tree_instr *ti = cni_iter.step();
	instruction *in = ti->instr();

	/* update def and use sets to reflect this instruction */
	note_instr_def_use(n, in, instr_def, instr_use);
    }
}

void
live_var_problem::note_instr_def_use(int n, instruction *in,
				     bit_set *def, bit_set *use)
{
    def->clear();
    use->clear();			     

    instr_def_use_fun(in, bit_man, def, use);
    
    bit_set *&d = (*defs)[n];
    bit_set *&u = (*uses)[n];

    /* Exclude new uses that have earlier defs.  Exclude new defs
     * that have earlier uses, including those in this instruction. */

    *use -= *d;
    *u += *use;
    *def -= *u;
    *d += *def;
}

/* live_var_problem::solver_ops() -- propagates the live_var
 * dataflow information through one block.  Returns TRUE if the
 * live_in set for this node n changed. */
boolean
live_var_problem::solver_ops(cfg_node *n)
{
    int nnum = n->number();
    bit_set *in = (*live_ins)[nnum];
    bit_set *out = (*live_outs)[nnum];

    /* out[n] = U in[succs(n)] */
    out->clear();

    cfg_node_list_iter s_iter(n->succs());
    while (!s_iter.is_empty())
	*out += *((*live_ins)[s_iter.step()->number()]);

    /* calculate in[n] = use[n] U (out[n] - def[n]) */
    bit_set t;             /* Separate declaration from assignment ... */
    t = *out;		   /* ... till bit_set gets a copy constructor */
    t -= *((*defs)[nnum]);
    t += *((*uses)[nnum]);

    /* update in set if necessary */
    if (t != *in) {
	*in = t;
	return TRUE;
    }
    return FALSE;
}

/*
 * Subfunctions of instruction_def_use. */
static void Def_or_use_by_operand(operand, boolean, operand_bit_manager *,
				  bit_set *);
static void Def_or_use_by_call(instruction *, operand_bit_manager *,
			       bit_set *, bit_set *);
static void Def_whole_convention(operand_bit_manager *, int, int, bit_set *);
static void Use_by_return(instruction *, operand_bit_manager *, bit_set *);


/* Instruction_def_use() -- Default functional argument for
 * live_var_problem.
 * Returns (via output variables) the def and use sets of the
 * given instruction.
 */
void
Instruction_def_use(instruction *in, operand_bit_manager *bit_man,
		    bit_set *def, bit_set *use)
{
    unsigned i;
    
    for (i = 0; i < in->num_dsts(); i++)
	Def_or_use_by_operand(in->dst_op(i), FALSE, bit_man, def);

    for (i = 0; i < in->num_srcs(); i++)
	Def_or_use_by_operand(in->src_op(i), TRUE,  bit_man, use);

    if (Is_call(in))
	Def_or_use_by_call(in, bit_man, def, use);

    else if (Is_return(in))
	Use_by_return(in, bit_man, use);
}

/* Def_or_use_by_operand() -- Subfunction of instr_def_use: does
 * one operand and returns TRUE unless the operand isn't a register
 * or symbol and deserves further attention if it's a source. */
static void
Def_or_use_by_operand(operand opd, boolean is_src,
		      operand_bit_manager *bit_man, bit_set *du)
{
    if (bit_man->insert(opd, du))
	return;
    if (is_src && Is_ea_operand(opd)) {
	static bit_set null;

	Instruction_def_use(opd.instr(), bit_man, &null, du);
	assert(null.is_empty());
    }
}

/* Def_or_use_by_call -- Helper for Instruction_def_use.  Augment
 * the use and def sets for a call instruction to reflect the arg
 * registers that the callee actually uses (taken from the
 * `k_regs_used' annotation that *gen provides) and all the
 * registers that the callee may define or destroy. */

static void
Def_or_use_by_call(instruction *in, operand_bit_manager *bit_man,
		   bit_set *def, bit_set *use)
{
    immed_list *iml = (immed_list *)in->peek_annote(k_regs_used);
    immed_list_iter imli(iml);
    while (!imli.is_empty()) {
	immed im = imli.step();
	assert(im.is_integer());	/* should be an int (abstract reg no) */

	int reg = im.integer();
	int bank = target_regs->describe(reg).bank;

  	bit_man->insert(operand(reg, type_void), use);
    }
    for (int b = 0; b < LAST_REG_BANK; b++) {
	Def_whole_convention(bit_man, b, ARG, def);
	Def_whole_convention(bit_man, b, RET, def);
	Def_whole_convention(bit_man, b, TMP, def);
	Def_whole_convention(bit_man, b, ASM_TMP, def);
    }
}

static void
Def_whole_convention(operand_bit_manager *bit_man, int bank, int conv,
		     bit_set *def)
{
    int n = target_regs->num_in(bank, conv);

    if (!n)
	return;

    int rr = REG(bank, conv, 0);
    do
	bit_man->insert(operand(rr++, type_void), def);
    while (--n);
}

static void
Use_by_return(instruction *in, operand_bit_manager *bit_man, bit_set *use)
{
    immed_list *instr_ret = (immed_list *)in->peek_annote(k_instr_ret);
    if (instr_ret && instr_ret->count() == 1) {
	immed instr_ret0 = (*instr_ret)[0];
	assert(instr_ret0.is_type());
	type_node *res_type = instr_ret0.type();
	int res_size = res_type->size();
	assert(res_type->op() == TYPE_PTR     /* FIXME when type ... */
	    || res_type->op() == TYPE_INT     /* ... masks are in */
	    || res_type->op() == TYPE_ENUM
	    || res_type->op() == TYPE_FLOAT);

	immed_list *regs_used = (immed_list *)in->peek_annote(k_regs_used);
	assert(regs_used && regs_used->count() == 1);
	immed iru = (*regs_used)[0];
	assert(iru.is_integer());
	bit_man->insert(operand(iru.integer(), res_type), use);
    }
}

void
Delete_bit_set_array(bit_set_array *a)
{
    if (a) {
	for (int i = 0; i < a->ub(); i++)
	    delete (*a)[i];
	delete a;
    }
}
