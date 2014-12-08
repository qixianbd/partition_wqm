/* file "annoteHelper.cc */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/* The following are a set of helper routines to help with the
 * manipulation of machSUIF-specific annotations.
 *
 * Please note that the definition of these annotations are in
 * machineInstr.{h,cc}.
 */


#include <suif1.h>
#include "machine_internal.h"


/******************************************************************
 *
 *  k_vsym_info annote helper routines.  The k_vsym_info annotation
 *  has the following fields (all INTs and in order):
 *
 *	sp_offset	stack pointer offset into frame 
 *	usage_cnt	total def and use count 
 *	param_reg	parameter reg used, if any
 *
 * Note that we use the SUIF annotation k_reg_num to store the
 * value of the hard register assigned, if any, to this var_sym.
 */

/* vsym_clear_hreg() -- this routine removes any k_reg_num annotation
 * associated with this variable symbol. */
void vsym_clear_hreg(var_sym *v)
{
    immed_list *iml = (immed_list *)v->get_annote(k_reg_num);
    if (iml != NULL) delete iml;
    assert(v->peek_annote(k_reg_num) == NULL);	/* sanity check */
    v->reset_reg();
}

/* vsym_get_hreg() -- this routine returns the hard register associated
 * with this variable symbol.  This routine uses the new-SUIF method
 * set_reg() and the k_reg_num annotation. */
int vsym_get_hreg(var_sym *v)
{
    /* sanity check -- must check for is_reg() before calling this routine */
    assert(v->is_reg());

    immed_list *iml = (immed_list *)v->peek_annote(k_reg_num);
    assert_msg(iml, ("vsym_get_hreg() -- no k_reg_num annote"));
    return (*iml)[0].integer();
}

/* vsym_set_hreg() -- this routine marks the variable symbol so that
 * the later ra0 passes know that this variable is kept in a hard
 * register.  This routine uses the new-SUIF method set_reg() and the
 * k_reg_num annotation. */
void vsym_set_hreg(var_sym *v, int r)
{
    immed_list *iml;

    /* some sanity checks */
    assert(!v->has_var_def());
    assert(!v->is_addr_taken() || v->is_param());

    /* allow only one k_reg_num annote */
    if ((iml = (immed_list *)v->get_annote(k_reg_num))) iml->pop();
    else iml = new immed_list();

    /* mark var_sym as register based */
    iml->append(immed(r));
    v->append_annote(k_reg_num, iml);
    v->set_reg();
}

/* vsym_get_sp_offset() -- this routine returns the stack pointer offset
 * associated with this variable symbol. */
int vsym_get_sp_offset(var_sym *v)
{
    immed_list *iml = (immed_list *)v->peek_annote(k_vsym_info);
    assert_msg(iml, ("vsym_get_sp_offset() -- no k_vsym_info annote"));
    return (*iml)[0].integer();
}

/* vsym_set_sp_offset() -- just what it says. */
void vsym_set_sp_offset(var_sym *v, int o)
{
    immed_list *iml;

    if ((iml = (immed_list *)v->peek_annote(k_vsym_info))) {
	/* pop off old value, discard old value, push new value */
	iml->pop();
	iml->push(immed(o));

    } else {	/* need to create the annote and initialize annote */
	iml = new immed_list();
	iml->append(immed(o));		/* set sp_offset */
	iml->append(immed(0));		/* default usage count */
	v->append_annote(k_vsym_info, iml);
    }
}

/* vsym_update_usage() -- this routine increments the usage count of
 * a var_sym by 1. */
void vsym_update_usage(var_sym *v)
{
    immed_list *iml;

    if ((iml = (immed_list *)v->get_annote(k_vsym_info))) {
	/* "pop off" old value, update value, push new value */
	immed im0 = iml->pop();
	immed im1 = iml->pop();
	iml->push(immed(im1.integer() + 1));
	iml->push(im0);

    } else {
	/* need to create the annote and initialize annote */
	iml = new immed_list();
	iml->append(immed(0));		/* default sp_offset */
	iml->append(immed(1));		/* set usage count */
    }
    v->append_annote(k_vsym_info, iml);
}

/* vsym_used() -- return the usage_cnt for this var_sym. */
int vsym_used(var_sym *v)
{
    immed_list *iml;
    if ((iml = (immed_list *)v->peek_annote(k_vsym_info)))
	return (*iml)[1].integer();
    else
	return 0;
}

/* vsym_set_preg() -- this routine records which parameter register
 * is used to pass this parameter var_sym. */
void vsym_set_preg(var_sym *v, int p)
{
    immed_list *iml;
    assert(v->is_param());	/* sanity check */

    if ((iml = (immed_list *)v->get_annote(k_vsym_info))) {
	/* "pop off" and save existing values for hreg and usage count */
	immed im0 = iml->pop();
	immed im1 = iml->pop();
	if (iml->count() > 0) iml->pop();	/* discard old value */
	iml->push(immed(p));
	iml->push(im1);
	iml->push(im0);

    } else {
	/* need to create the annote and initialize annote */
	iml = new immed_list();
	iml->append(immed(0));		/* default sp_offset */
	iml->append(immed(0));		/* set usage count */
	iml->append(immed(p));
    }
    v->append_annote(k_vsym_info, iml);
}

/* vsym_passed_in_preg() -- this routine returns TRUE if the var_sym
 * is a parameter that is passed in a parameter register.  Note that
 * the existence of an entry in the location for the parameter reg
 * value means that the var_sym is passed in a parameter register. */
boolean vsym_passed_in_preg(var_sym *v)
{
    immed_list *iml = (immed_list *)v->peek_annote(k_vsym_info);
    assert(iml != NULL);				/* oops */

    if (v->is_param() && (iml->count() > 2)) return TRUE;
    
    return FALSE;
}

/* vsym_get_preg() -- this routine returns the parameter register
 * identifier used to pass this var_sym, assuming it is a parameter
 * and it is passed in a register. */
int
vsym_get_preg(var_sym *v)
{
    immed_list *iml = (immed_list *)v->peek_annote(k_vsym_info);
    assert_msg(iml, ("vsym_get_preg() -- no k_vsym_info annote"));
    assert(v->is_param());
    assert(iml->count() > 2);
    return (*iml)[2].integer();
}

/* vsym_get_auto_sym() -- search the local symbol list for an auto
 * variable with the offset specified in the parameter list.  Return
 * a NULL sym_addr if no match. */
sym_addr
vsym_get_auto_sym(base_symtab *bst, int stack_frame_size, int sp_offset)
{
    sym_node_list_iter sym_iter(bst->symbols());
    while (!sym_iter.is_empty()) {
	var_sym *v = (var_sym *)sym_iter.step();
	if (v->is_var() && v->is_auto()) {
	    int search_o = sp_offset; // bytes
	    int v_o = vsym_get_sp_offset(v) + stack_frame_size; // bytes

	    // Complicated case of a ref into a structure/array.  Remember
	    // that these offsets are negative numbers!  This incorporates
	    // the simple case of (v_o == search_o).
	    int size_of_v = v->type()->size() >> SHIFT_BITS_TO_BYTES;
	    if ((v_o <= search_o) && (search_o < (v_o + size_of_v)))
		return sym_addr(v, ((search_o - v_o) << SHIFT_BITS_TO_BYTES));
	}
    }
    return sym_addr((var_sym *)NULL,0);
}
