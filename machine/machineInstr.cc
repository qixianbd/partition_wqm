/* file "machineInstr.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,96 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*
 * Generic machine instruction implementation.
 *
 * When adding a new machine architecture, **nothing** needs to
 * be changed in this implementation file.  The **only** place
 * where you need to make additions is in machineUtil.cc.
 */


#include <suif1.h>
#include "machine_internal.h"


/*
 * Useful constants.
 */
char *k_null_string;

/*
 * New flat annotations used by all machine instructions.
 */
char *k_comment;
char *k_machine_instr;
char *k_hint;
char *k_reloc;
char *k_instr_op_exts;
char *k_instr_xx_sources;
char *k_instr_bj_target;
char *k_instr_mbr_tgts;
char *k_instr_ret;
char *k_incomplete_proc_exit;
char *k_mbr_index_def;
char *k_regs_used;
char *k_regs_defd;
char *k_is_store_ea;
char *k_proc_entry;
char *k_target_arch;
char *k_next_free_reloc_num;
char *k_vreg_manager;
char *k_stack_frame_info;
char *k_vsym_info;
char *k_header_trailer;

/*
 * Global variables.
 */
boolean skip_printing_of_unwanted_annotes = FALSE;
nonprinting_annotes_list *nonprinting_annotes;

/* Print_data_label() -- helper function to print out data labels
 * since we don't have an instruction class capable of holding
 * data labels (and we don't need one). */
void Print_data_label(FILE *o_fd, sym_node *s, char ending)
{
    Print_symbol(o_fd, s);
    fprintf(o_fd, "%c\n", ending);
}


/* Print_raw_immed() -- helper function to print out immediate values
 * in the rawest manner, i.e. no extra stuff. */
void Print_raw_immed(FILE *o_fd, immed im)
{
    char *p;

    switch (im.kind()) {
      case im_int:
	fprintf(o_fd, "%d", im.integer());
	break;
      case im_extended_int:
	fprintf(o_fd, "%s", im.ext_integer());
	break;
      case im_string:
	p = im.string();
        assert(p);
        fputs(p, o_fd);
	break;
      case im_float:
	fprintf(o_fd, "%.18e", im.flt());
	break;
      case im_extended_float:
	fprintf(o_fd, "%s", im.ext_flt());
	break;
      case im_symbol:
	Print_symbol(o_fd, im.symbol());
	if (im.offset() != 0) {
	    /* insert '+' if offset is not negative */
	    if (im.offset() > 0) fprintf(o_fd, "+");
	    /* shift offset from bits to bytes */
	    fprintf(o_fd, "%d", im.offset() >> 3);
	}
	break;
      case im_type:
	(im.type())->print(o_fd);
	break;
      case im_undef:
	fputs("undef", o_fd);
	break;
      default:
	assert_msg(FALSE, ("Print_raw_immed() -- unhandled case"));
    }
}


/* Print_symbol() -- helper function to print SUIF symbols without
 * the leading '.' -- typically not wanted in assembly code.  This
 * routine is a slight modification of sym_node::print(). */
void Print_symbol(FILE *o_fd, sym_node *s)
{
    char *cn = s->parent()->chain_name();
    if ((cn == NULL) || (cn[0] == '\0'))
	fprintf(o_fd, "%s", s->name());
    else {
	if (cn[0] == '.') cn++;
	fprintf(o_fd, "%s.%s", cn, s->name());
    }
}


/* Print_milist() -- helper function to print a machine instruction list. */
void Print_milist(FILE *o_fd, milist *l)
{
    milist_iter mili(l);
    while (!mili.is_empty()) (mili.step())->print(o_fd);
}


/* ------------ Helper query routines  -------------- */

/* Immed_operand() -- constructs an immediate operand by creating
 * an OPER_INSTR kind of operand containing a ldc instruction. */
operand Immed_operand(immed &im, type_node *t)
{
    return operand(new in_ldc(t, operand(), im));
}


/* Is_ea_operand() -- returns TRUE if the operand is an effective address
 * calculation (i.e. not an immediate operand).  The only way to tell
 * if a ldc instruction represents an effective address calculation and
 * not a simple immediate is to check the type of the ldc instruction.
 * If it's a pointer type, then the ldc represents an effective address
 * calculation.  If not, it's a simple immediate value. */
boolean Is_ea_operand(operand o)
{
    if (o.is_instr()) {
        if (o.type()->is_ptr()) return TRUE;
    }
    return FALSE;
}

/* ------------ Other operand helpers  -------------- */

/* Map_operand() -- Recursive helper function that applies
 * "fun" to each operand in the instruction "in", except
 * for back-pointer operands in EA trees.  Sets each operand
 * to the return value from "fun".
 * Function "fun" takes two arguments: a flag that is true only
 * for source operands and an arbitrary pointer provided as the
 * third argument of Map_operand.
 */
void
Map_operand(instruction *in, operand (*fun)(operand, boolean, void *), void *x)
{
    unsigned i;

    for (i = 0; i < in->num_srcs(); i++) {
	operand o = in->src_op(i);
	if (o.is_instr()) {
	    Map_operand(o.instr(), fun, x);
	    operand o2 = fun(o, TRUE, x);
	    assert(o == o2);
	} else {
	    in->set_src_op(i, fun(o, TRUE, x));
	}
    }
    for (i = 0; i < in->num_dsts(); i++) {
	operand o = in->dst_op(i);
	if (!o.is_instr())
	    in->set_dst(i, fun(o, FALSE, x));
    }
}


/* ------------ methods for mproc_symtab hack -------------- */

static int size_of_vrmap = 0;	
static int *vrmap = NULL; 
static mproc_symtab *cur_mtab = NULL; 

/* Grow_operand() -- Helper function for mproc_symtab::renumber_vregs()
 * that grows the set of vregs and the mapping to new vreg numbers during
 * the first pass. */
static operand
Grow_operand (operand o, boolean, void *)
{
    if (o.is_virtual_reg()) {
	int vmi = -o.reg();

	/* check to see if index in range */
	if (vmi >= size_of_vrmap) {
	    /* oops, realloc vrmap array */
	    vrmap = (int *)realloc(vrmap, (vmi+20) * sizeof(int));
	    for (int i = size_of_vrmap; i < (vmi+20); i++)
		vrmap[i] = 0;
	    size_of_vrmap = vmi + 20;
	}

	/* update map only if 1st time we've seen this vreg */
	if (vrmap[vmi] == 0) 
	    vrmap[vmi] = cur_mtab->next_vreg_num();
    } 

    /* else don't care */
    return o; 
}

/* Renumber_operand() -- Helper function for mproc_symtab::renumber_vregs()
 * that applies the remapping of virtual register numbers during the 
 * second pass. */
static operand
Renumber_operand (operand o, boolean, void *)
{
    if (o.is_virtual_reg()) {
	int vmi = -o.reg();		/* remember that vregs are negative */
	assert(vmi < size_of_vrmap);
	int new_vr = vrmap[vmi];
	assert(new_vr < 0);		/* otherwise no def */
	return operand(new_vr, o.type());
    } 

    /* no side effect */
    return o; 
}

/* mproc_symtab::renumber_vregs() -- this routine renumbers the virtual
 * register used in the tree_proc body.  This routine should be called
 * before Write_machine_proc(). */
void
mproc_symtab::renumber_vregs()
{
    int i;

    /* create and initialize vreg map */
    cur_mtab = this; 
    size_of_vrmap = -next_vrnum + 10;	/* few more for good measure */
    vrmap = (int *)malloc(size_of_vrmap * sizeof(int));
    for (i = 0; i < size_of_vrmap; i++) vrmap[i] = 0;

    /* reset the numbering in the virtual-register manager */
    set_vreg_num(-1);

    /* grab the tree_node_list from the tree_proc */
    tree_node_list *tnl = pst()->block()->body();
    tree_node_list_iter tnli(tnl);

    /* walk the list first to see and update all of the vreg definitions */
    while (!tnli.is_empty()) {
	tree_node *tn = tnli.step();
	assert_msg((tn->kind() == TREE_INSTR), 
		("renumber_vregs() -- found an unexpected kind of tree_node"));
	Map_operand(((tree_instr *)tn)->instr(), Grow_operand, NULL); 
    }

    /* now walk the list to update all of the vreg uses */
    tnli.reset(tnl);
    while (!tnli.is_empty()) {
	tree_node *tn = tnli.step();
	assert_msg((tn->kind() == TREE_INSTR), 
		("renumber_vregs() -- found an unexpected kind of tree_node"));
	Map_operand(((tree_instr *)tn)->instr(), Renumber_operand, NULL); 
    }

    free(vrmap);
}


/* ------------ Input/output stream routines -------------- */

/* Convert_to_mi() -- helper routine for Read_machine_proc(). */
static machine_instr *Convert_to_mi(instruction *i)
{
    unsigned j;
    int m_opcode;
    machine_instr *mi = NULL;
    immed_list *il = (immed_list *)i->get_annote(k_machine_instr);
    assert(il);

    switch (i->opcode()) {
      case io_gen:					/* generic */
	assert(((in_gen *)i)->name() != k_suif);
	m_opcode = (*il)[1].integer();	/* get opcode */

	/* create machine instruction based on in_gen*/
	switch (which_mformat(m_opcode)) {
	  case mif_xx:
	    mi = new mi_xx(m_opcode);
	    if ((il = (immed_list *)i->get_annote(k_instr_xx_sources))) {
		while (!il->is_empty())
		    ((mi_xx *)mi)->append_operand(il->pop());
		delete il;
	    }
	    break;

	  case mif_lab:
	    assert_msg(FALSE, ("Convert_to_mi() -- found io_gen label"));

	  case mif_bj:
	    mi = new mi_bj(m_opcode, NULL);
	    if ((il = (immed_list *)i->get_annote(k_instr_bj_target))) {
		/* branch/jump has symbolic target, transfer it */
		sym_node *t = (*il)[0].symbol();
		((mi_bj *)mi)->set_target(t);
		delete il;
	    }
	    break;

	  case mif_rr:
	    mi = new mi_rr(m_opcode);
	    break;

	  default:
	    assert_msg(FALSE, ("Convert_to_mi() -- unknown instr format"));
	}

	/* transfer result_type */
	mi->set_result_type(i->result_type());

	/* transfer generic destination operands */
	mi->set_num_dsts(i->num_dsts());
	for (j = 0; j < mi->num_dsts(); j++) {
	    assert(!i->dst_op(j).is_instr());
	    mi->set_dst(j, i->dst_op(j));
	}

	/* transfer generic source operands */
	mi->set_num_srcs(i->num_srcs());
	for (j = 0; j < i->num_srcs(); j++) {
	    if (i->src_op(j).is_instr()) {
		/* transfer immediate operand or expression tree
		 * representing an effective address operand */
		instruction *eai = i->src_op(j).instr();
		i->src_op(j).remove();		/* clear pointers */
		i->set_src_op(j, operand());
		mi->set_src_op(j, operand(eai));

	    } else mi->set_src_op(j, i->src_op(j));
	}

	break;

      case io_lab:					/* label */
	mi = new mi_lab((*il)[1].integer(), (in_lab *)i);
	break;

      default:
	assert_msg(FALSE, ("Convert_to_mi() -- unexpected SUIF opcode"));
    }

    assert(mi);		/* sanity check */

    /* transfer any remaining annotations */
    i->copy_annotes(mi);

    return mi;
}


/* Read_machine_proc() -- helper routine to generate a tree_proc body full
 * of machine instructions from a tree_proc body of generic instructions
 * with machine instruction annotations.  This routine can handle
 * either flat list or the funny machine expression lists.
 * 
 * This routine returns a pointer to a ``machine proc_symtab'' which is
 * just a hack wrapper around the SUIF proc_symtab since we need to
 * have a place to put the virtual register manager.  This functionality
 * should exist in the basesuif classes.
 *
 * This routine can also handle the case where the tree_proc body contains
 * no machine instructions, i.e. safe to run on SUIF instructions.
 */
mproc_symtab *
Read_machine_proc (proc_sym *psym, boolean exp_trees, 
		   boolean use_fortran_form)
{
    immed_list *il;
    int max_vn = 0;		/* sanity check on k_virtual_reg_manager */
    unsigned i;

    /* perform real read */
    psym->read_proc(exp_trees, use_fortran_form);

    /* process proc body */
    tree_node_list_iter tnli(psym->block()->body());
    while (!tnli.is_empty()) {
	tree_node *tn = tnli.step();
	tree_instr *ti;
	instruction *in;

	switch (tn->kind()) {
	  case TREE_INSTR:
	    ti = (tree_instr *)tn;
	    in = ti->instr();

	    /* Find machine instructions by searching for a
	     * k_machine_instr annotation. */
	    if ((il = (immed_list *)in->peek_annote(k_machine_instr))) {
		machine_instr *mi = Convert_to_mi(in);

		/* cannot update in place -- fake update it */
		ti->remove_instr(in);
		ti->set_instr(mi);
		delete in;
		in = mi;
	    }

	    /* Walk the destination operand list looking for virtual
	     * registers.  We only need to walk this list because
	     * mproc_symtab::renumber_vregs(), if called, guaranteed
	     * that there were no uses without prior defintions. */
	    for (i = 0; i < in->num_dsts(); i++) {
		operand o = in->dst_op(i);
		if (o.is_virtual_reg()) {
		    int r = o.reg();
		    max_vn = (r < max_vn) ? r : max_vn;
		    /* remember that vregs are negative */
		}
	    }
	    break;

	  default:
	    /* Currently we do not handle anything but tree_instrs,
	     * but this routine can be easily extended to be general. */
	    assert_msg(FALSE, ("Read_machine_proc() -- "
			       "unexpected kind of tree_node"));
	}
    }

    /* Read the k_vreg_manager annote, if it exists, and create the
     * mproc_symtab hack.  The annotation may not exist if this
     * is not a machsuif procedure. */
    mproc_symtab *mpst = new mproc_symtab(psym->block()->proc_syms());
    if ((il=(immed_list *)psym->block()->get_annote(k_vreg_manager))) {
	assert(il->count() == 1);
	int nvn = (*il)[0].integer();
	assert_msg(((max_vn - 1) == nvn),
		   ("Read_machine_proc() -- next virtual reg number in "
		    "annote doesn't match that in code"));
	mpst->set_vreg_num(nvn);
    } else {
	assert_msg((max_vn == 0), ("Read_machine_proc() -- found a virtual "
		    "reg even though no k_vreg_manager annote"));
    }

    return mpst;
}


/* Make_generic -- This routine translates all machine instructions into
 * the SUIF in_gen equivalent instruction.  Basically, it adds annotations
 * to keep the information that is not representable in the SUIF library.
 * Whenever possible, it attempts to perform this translation in place.  If
 * successful, this routine returns a NULL pointer.
 *
 * This routine is safe.  It will only translate machine instructions.  All
 * other SUIF instructions will be left unchanged. */
static instruction *Make_generic(instruction *in)
{
    if (in->architecture() == k_suif) return NULL;

    if (in->instruction::format() == inf_gen) {
	in_gen *g = (in_gen *)in;
	machine_instr *mi = (machine_instr *)g;

	mi_bj *b;		/* in case they're needed */
	mi_xx *x;
	in_lab *lb;

	/* Create machine_instr annote holding base class info. */
	immed_list *il = new immed_list();
	il->append(immed(mi->architecture()));
	il->append(immed(mi->opcode()));
	il->append(immed(mi->op_string()));	// for debugging only
	mi->append_annote(k_machine_instr, il);

	/* The destination operands, result_type, and source operands
	 * remain unchanged, as do any annotations (e.g. the comment
	 * field, etc.). */

	switch (mi->format()) {
	  case mif_xx:
	    /* add mi_xx specific annotations -- update in place */
	    x = (mi_xx *)mi;
	    il = new immed_list();
	    while (x->has_operands()) il->append(x->pop_operand());
	    if (!il->is_empty()) x->append_annote(k_instr_xx_sources, il);
	    return NULL;

	  case mif_lab:
	    /* translate a label instruction into a SUIF in_lab 
	     * instruction.  Note that I do not return an in_gen
	     * for labels! */
	    lb = new in_lab(((mi_lab *)mi)->label());
	    mi->copy_annotes(lb);
	    return lb;
		
	  case mif_bj:
	    /* update in place */
	    b = (mi_bj *)mi;
	    if (!b->is_indirect()) {
		/* transfer target label -- update in place */
		il = new immed_list();
		il->append(immed(b->target()));
		b->append_annote(k_instr_bj_target, il);
	    }
	    return NULL;

	  case mif_rr:
	    return NULL;	/* nothing more to do -- update in place */

	  default:
	    assert_msg(FALSE, ("Make_generic -- "
			       "encountered an unimplemented machine format"));
	}

    } else assert_msg(FALSE, ("Make_generic() -- "
	   "expected all machine instructions to be in_gen's"));

    return NULL;	/* to keep the compiler quiet */
}


/* Write_machine_proc -- helper routine to write a tree_proc full of
 * machine instructions to a SUIF file.  This routine also creates
 * the k_vreg_manager annote on the tree_proc.
 * 
 * This routine is safe since it can be run on procedure bodies without
 * machine instructions. */
void 
Write_machine_proc (proc_sym *psym, file_set_entry *fse)
{
    int max_vn = 0;
    unsigned i;

    /* process proc body -- looking to convert machine instructions */
    tree_node_list_iter tnli(psym->block()->body());
    while (!tnli.is_empty()) {
	tree_node *tn = tnli.step();
	tree_instr *ti;
	instruction *in0, *in2;
	switch (tn->kind()) {
	  case TREE_INSTR:
	    ti = (tree_instr *)tn;
	    in0 = ti->instr();
        //((machine_instr*)in0)->print(stdout);

	    if ((in2 = Make_generic(in0))) {
		/* did not update in place -- fake update in place */
		ti->remove_instr(in0);
		ti->set_instr(in2);
		delete in0;
		in0 = in2;
	    }

	    /* Walk the destination operand list looking for virtual
	     * registers.  We only need to walk this list because
	     * mproc_symtab::renumber_vregs(), if called, guaranteed
	     * that there were no uses without prior defintions. 
	     * Notice that I don't trust the number in the mproc_symtab,
	     * and even if I did, I cannot get it always. */
	    for (i = 0; i < in0->num_dsts(); i++) {
		operand o = in0->dst_op(i);
		if (o.is_virtual_reg()) {
		    int r = o.reg();
		    max_vn = (r < max_vn) ? r : max_vn;
		    /* remember that vregs are negative */
		}
	    }
	    break;

	  default:
	    /* Currently we do not handle anything but tree_instrs,
	     * but this routine can be easily extended to be general. */
	    assert_msg(FALSE, ("Write_machine_proc() -- "
			       "unexpected kind of tree_node"));
	}
    }

    /* create k_vreg_manager annotation if necessary */
    if (max_vn < 0) {
	/* procedure uses virtual registers */
	immed_list *il = new immed_list();
	il->append(immed(max_vn - 1));
	psym->block()->append_annote(k_vreg_manager, il);
    }

    /* perform actual write after a bit of bookkeeping */
    psym->block()->clear_numbers();
    psym->block()->number_instrs();
    psym->write_proc(fse);
//    psym->flush_proc();
}


/* ------------ machine_instr methods -------------- */

/* machine_instr::op_string() is kept in machineUtil.cc */

/* machine_instr::append_comment() -- the current implementation only
 * allows a single comment per instruction.  Thus, this routine checks
 * for a comment (and deletes it if found) before adding the current
 * comment. */
void machine_instr::append_comment(immed_list *il)
{
    if (get_annote(k_comment) != NULL) {
	warning_line(NULL, "machine_instr::append_comment() -- "
		     "instruction already had a comment, overwriting");
    }
    append_annote(k_comment, il);
}


/* machine_instr::set_opcode() -- set the appropriate machine state
 * when setting the machine opcode. */
void
machine_instr::set_opcode(if_ops o)
{
    set_name(which_architecture(o));
    m_op = o;
}


/* unwanted_annote() -- list of annote names that we don't want
 * printed by default.  Corresponds to the flag
 * skip_printing_of_unwanted_annotes. */
static boolean
unwanted_annote(char *aname)
{
    nonprinting_annotes_list_iter npali(nonprinting_annotes);
    while (!npali.is_empty()) {
	char *n = npali.step();
	if (aname == n) return TRUE;
    }
    return FALSE;
}

/* machine_instr::print_comment() --  print out comment and any other
 * annotations on a machine instruction. */
void machine_instr::print_comment(FILE *o_fd, char *comment_char)
{
    /* print out the comment, if any */
    immed_list *cl = (immed_list *)peek_annote(k_comment);
    if (cl) {
	immed_list_iter ili(cl);
	fprintf(o_fd, "\t%s", comment_char);
	while (!ili.is_empty()) {
	    immed im = ili.step();
	    fprintf(o_fd, " ");
	    Print_raw_immed(o_fd, im);
	}
    }
    if (cl || !Is_null(this)) fprintf(o_fd,"\n");

    /* print out any annotations */
    if (are_annotations()) {
	annote_list_iter ali(annotes());
	while (!ali.is_empty()) {
	    annote *ap = ali.step();
	    if ((ap->name() == k_comment)
	     || (skip_printing_of_unwanted_annotes
		 && unwanted_annote(ap->name()))) {
		/* print nothing */
	    } else {
		fprintf(o_fd," %s ", comment_char);
		ap->print(o_fd);
		fprintf(o_fd,"\n");
	    }
	}
    }
    fflush(o_fd);
}

/* machine_instr::print() -- default printout for base class */
void machine_instr::print(FILE *o_fd)
{
    fprintf(o_fd, "\t%s\t", op_string());
    print_comment(o_fd, "#");		/* prints annotations too */
}


/* machine_instr::clone_base() -- Set the common fields in the
 * newly created clone machine instruction.  This is called by
 * the clone_helper methods in the derived machine instruction
 * classes. */
void machine_instr::clone_base(machine_instr *i, replacements *r,
					 boolean no_copy)
{
    instruction::clone_base(i, r, no_copy); 
    i->set_opcode(opcode());
    i->set_name(name()); 
    i->set_num_srcs(num_srcs()); 
    for (unsigned j = 0; j < num_srcs(); j++)
	i->set_src_op(j, src_op(j).clone_helper(r, no_copy)); 
}


/* ------------ derived class methods -------------- */

/* mi_lab::mi_lab() -- constructors */
mi_lab::mi_lab(mi_ops o, label_sym *s):machine_instr(which_architecture(o),o)
{
    lab = s;
}

mi_lab::mi_lab(mi_ops o, in_lab *i):machine_instr(which_architecture(o),o)
{
    lab = i->label();
    copy_annotes(i);
}

/* mi_lab::clone_helper() -- clone a label instruction. */
instruction *mi_lab::clone_helper(replacements *r, boolean no_copy)
{
    mi_lab *result = this;
    if (!no_copy) result = new mi_lab();
    clone_base(result, r, no_copy);
    result->set_label((label_sym *)label()->clone_helper(r));
    if (!no_copy) {
        /* make sure that it is not a duplicate label */
        assert_msg(result->label() != label(),
                   ("mi_lab::clone_mi_helper -- label \"%s\" has not been "
                    "replaced", label()->name()));
    }
    return result;
}

/* mi_lab::find_exposed_refs() -- find refs to promote on cloning */
void mi_lab::find_exposed_refs(base_symtab *dst_scope, replacements *r)
{
    in_gen::find_exposed_refs(dst_scope, r); 
    r->add_sym_ref(label(), dst_scope, TRUE); 
}

/* mi_lab::print() is kept in machineUtil.cc */


/*** -------------------- ***/

/* mi_bj::mi_bj() -- null constructor */
mi_bj::mi_bj() : mi_rr(io_jmp)
{
    targ = NULL;
    /* set_num_dsts(1); done in in_gen constructor */
    set_num_srcs(2);
}

/* mi_bj::mi_bj() -- constructor for mif_bj format.  Note that you
 * cannot set the target with a non-zero offset.  You must do that
 * manually using the method set_target(). */
mi_bj::mi_bj(mi_ops o, sym_node *t, operand d, operand s1,
	     operand s2) : mi_rr(o, d, s1, s2)
{
    if (t) set_target(t);
    else targ = NULL;
}

mi_bj::mi_bj(mi_ops o, instruction *ea, sym_node *t, operand d, operand s1,
	     operand s2) : mi_rr(o, ea, s1, s2)
{
    /* set_num_dsts(1); done in in_gen constructor */
    set_dst(d);
    set_result_type(d.type());		/* by default */

    if (t) set_target(t);
    else targ = NULL;
}

/* mi_bj::clone_helper() -- clone a bj instruction. */
instruction *mi_bj::clone_helper(replacements *r, boolean no_copy)
{
    mi_bj *result = this;
    if (!no_copy) result = new mi_bj();
    clone_base(result, r, no_copy);
    if (target()) result->set_target(target()->clone_helper(r));
    return result;
}

/* mi_bj::find_exposed_refs() */
void
mi_bj::find_exposed_refs (base_symtab *dst_scope, replacements *r)
{
    in_gen::find_exposed_refs(dst_scope, r);
    if (target()) r->add_sym_ref(target(), dst_scope);
}

/* mi_bj::print() is kept in machineUtil.cc */


/*** ---------------- ***/

/* mi_rr::mi_rr() -- null constructor */
mi_rr::mi_rr() : machine_instr(k_null_string, io_nop)
{
    /* set_num_dsts(1); done in in_gen constructor */
    set_num_srcs(2);
}

/* mi_rr::mi_rr() -- constructor for mif_rr format */
mi_rr::mi_rr(mi_ops o, operand d, operand s1,
	     operand s2) : machine_instr(which_architecture(o),o)
{
    /* set_num_dsts(1); done in in_gen constructor */
    set_dst(d);
    set_result_type(d.type());		/* by default */

    /* setup machine_instr sources */
    set_num_srcs(2);		/* also creates shadow srcs in in_gen */
    set_src_op(0,s1);
    set_src_op(1,s2);
}

/* mi_rr::mi_rr() -- constructor for mif_rr format that writes memory.
 * By convention, we keep the EA for a store in srcs[0]. */
mi_rr::mi_rr(mi_ops o, instruction *ea, operand s1, operand s2) :
	       machine_instr(which_architecture(o),o)
{
    /* dsts[0] = operand(); done in in_gen constructor */
    set_result_type(s1.type());
    set_num_srcs(3);
    set_store_addr_op(0,operand(ea));	/* store effective-address operand */
    set_src_op(1,s1);			/* typical store source */
    set_src_op(2,s2);
}

/* mi_rr::store_addr_op() -- helper method that retrieves operand n
 * and makes sure that it is a store effective address calculation. */
operand mi_rr::store_addr_op(unsigned n)
{
    int i;
    operand o = src_op(n);

    /* Check for annotation -- prefer it on the operand (but not the
     * operand contents since this annote is not a property of the
     * contents but of the operand).  And check that the operand is
     * an EA calculation. */
    immed_list *iml = (immed_list *)this->peek_annote(k_is_store_ea);
    if (iml && Is_ea_operand(o)) {
	/* Ok so far.  Now see if n matches an item in the immed_list. */
	for (i = 0; i < iml->count(); i++) {
	    if ((*iml)[i].integer() == (int)n) return o;
	}
    }
    assert_msg(FALSE,("store_addr_op() -- src_op(%d) is NOT a store EA", n));
    return operand();
}

/* mi_rr::set_store_addr_op() -- helper method that follows ensures the
 * correct annotation is created for a store effective address calculation. */
void mi_rr::set_store_addr_op(unsigned n, operand r)
{
    /* sanity check for correct form of operand r */
    assert(Is_ea_operand(r));

    /* first make sure that there isn't an entry for this operand
     * in a possibly existing k_is_store_ea annotation */
    remove_store_addr_op(n);

    /* create k_is_store_annote entry for operand n */
    immed_list *iml = (immed_list *)this->get_annote(k_is_store_ea);
    if (iml == NULL) iml = new immed_list();
    iml->append(immed(n));
    this->append_annote(k_is_store_ea, iml);

    /* write operand */
    set_src_op(n,r);
}

/* mi_rr::remove_store_addr_op() -- helper method that clears the annotation
 * state for the store effective address calculation in operand n.  The
 * operand can already be NULL (and no annotation) to support the
 * set_store_addr_op() method's use of this method. */
void mi_rr::remove_store_addr_op(unsigned n)
{
    immed_list *iml = (immed_list *)this->get_annote(k_is_store_ea);

    /* set operand n to null */
    operand o = src_op(n);
    if (!o.is_null()) {
	if (o.is_instr()) delete o.instr();
	set_src_op(n, operand());
    }    

    /* do the rest only if iml exists */
    if (iml == NULL) return;

    /* find immed_list element to delete */
    immed_list_iter imli(iml);
    while (!imli.is_empty()) {
	immed im = imli.step();
	if (im.integer() == (int)n) {
	    iml->remove(imli.cur_elem());
	    break;
	}
    }

    /* put the annotation back only if needed */
    if (iml->count() == 0) delete iml;
    else this->append_annote(k_is_store_ea, iml);
}

/* mi_rr::clone_helper() -- clone an rr instruction. */
instruction *mi_rr::clone_helper(replacements *r, boolean no_copy)
{
    mi_rr *result = this;
    if (!no_copy) result = new mi_rr();
    clone_base(result, r, no_copy);
    return result;
}

/* mi_rr::find_exposed_refs() -- */
void mi_rr::find_exposed_refs (base_symtab *dst_scope, replacements *r)
{
    in_gen::find_exposed_refs(dst_scope, r);
}

/* mi_rr::print() is kept in machineUtil.cc */


/*** ---------------- ***/

/* mi_xx::mi_xx() -- constructor for mif_xx format */
mi_xx::mi_xx(mi_ops o) : machine_instr(which_architecture(o), o)
{
    /* empty by design */
}

mi_xx::mi_xx(mi_ops o, immed i) : machine_instr(which_architecture(o), o)
{
    append_operand(i);
}

/* mi_xx::clone_helper() -- clone a pseudo op */
instruction *mi_xx::clone_helper(replacements *r, boolean no_copy)
{
    mi_xx *result = this;
    if (!no_copy) result = new mi_xx();
    clone_base(result, r, no_copy);
    immed_list *new_il = il.clone_helper(r, no_copy);
    if (!no_copy) {
	result->il.grab_from(new_il);
        delete new_il;
    }
    return result;
}

/* mi_xx::find_exposed_refs() */
void mi_xx::find_exposed_refs(base_symtab *dst_scope, replacements *r)
{
    in_gen::find_exposed_refs(dst_scope, r); 
    il.find_exposed_refs(dst_scope, r); 
}

/* mi_xx::print() is kept in machineUtil.cc */
