/* file "eaHelper.cc */

/*  Copyright (c) 1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/* The following set of helper routines assist in the creation and 
 * manipulation of effective-address calculations in Machine SUIF. */

#include <suif1.h>
#include "machine_internal.h"

/* ---------- Creation helpers ---------- */

/* New_ea_base_p_disp() -- this routine creates a base+displacement
 * effective-address calculation.  The base must be a register or
 * symbol operand; anything else causes an assertion failure.  The
 * displacement is always interpreted as a signed number and it
 * is in **bytes**. */
instruction *
New_ea_base_p_disp(operand b, long d)
{
    // check base operand
    assert(b.is_reg() || b.is_symbol());

    immed d_im(d);	// long converted to correct size by immed
    return new in_rrr(io_add, type_ptr, operand(), b,
		      Immed_operand(d_im, type_signed));
}

instruction *
New_ea_base_p_disp(operand b, immed di)
{
    // check base operand
    assert(b.is_reg() || b.is_symbol());
    return new in_rrr(io_add, type_ptr, operand(), b,
		      Immed_operand(di, type_signed));
}


/* New_ea_symaddr() -- this routine creates a simple relocatable-symbol
 * effective-address calculation.  The form of this calculation is
 * r_symbol+displacement.  The displacement is a positive offset (required
 * by the underlying sym_addr).  You specify the displacement in bytes,
 * though it is stored in the sym_addr as a **bit** displacment.
 *
 * The type of the in_ldc is always type_ptr.  We could be more exact,
 * but it seems unnecessary at this point.  The only time it really
 * matters is in a load-address operation.  In these cases, the real
 * type can be easily generated from taking the symbol type and making
 * a pointer to that type. */
instruction *
New_ea_symaddr(sym_node *s, unsigned d)
{
    return new in_ldc(type_ptr, operand(),
		      immed(s, d << SHIFT_BITS_TO_BYTES));
}

instruction *
New_ea_symaddr(immed si)
{
    assert(si.is_symbol());
    return new in_ldc(type_ptr, operand(), si);
}


/* New_ea_indexed_symaddr() -- this routine creates an indexed
 * relocatable-address calculation.  It is assumed that the
 * index operand is a register.  To get an address, the contents
 * of the index register are added to the relocatable symbol
 * address constant, plus an optional additional displacement.
 * The displacement is signed and in **bytes**. */
instruction *
New_ea_indexed_symaddr(operand i, sym_node *s, long d)
{
    // check index operand
    assert(i.is_reg());

    immed d_im(d);	// long converted to correct size by immed

    // Create the rhs subexpression first.  The relocatable symbol
    // operand is encapsulated in an io_ldc because we really want the
    // address of this symbol (plus it may be something other than 
    // a var_sym).
    instruction *subtree = new in_rrr(io_add, type_ptr, operand(),
				      New_ea_symaddr(s),
				      Immed_operand(d_im, type_signed));
    return new in_rrr(io_add, type_ptr, operand(), i, operand(subtree));
}


/* New_ea_base_p_indexS_p_disp() -- this routine creates a
 * base+(index*scale)+displacement effective-address calculation.  The
 * base and index must be register or symbol operands; anything else
 * causes an assertion failure.  The displacement is always interpreted
 * as a signed number and it is in **bytes**.  The scale is unsigned
 * and also in **bytes**. */
instruction *
New_ea_base_p_indexS_p_disp(operand b, operand i, unsigned s, long d)
{
    // check base and index operands
    assert(b.is_reg() || b.is_symbol());
    assert(i.is_reg() || i.is_symbol());

    instruction *e;
    immed s_im(s);
    e = new in_rrr(io_mul, type_ptr, operand(), i,
		Immed_operand(s_im, type_unsigned));
    e = new in_rrr(io_add, type_ptr, operand(), b, operand(e));
    immed d_im(d);	// long converted to correct size by immed
    return new in_rrr(io_add, type_ptr, operand(), operand(e),
		      Immed_operand(d_im, type_signed));
}


/* ---------- Query helpers ---------- */

enum ea_kinds {
    EA_UNKNOWN,
    EA_BASE_P_DISP,
    EA_SYMADDR,
    EA_INDEXED_SYMADDR,
    EA_INDEXS_P_DISP,
    EA_BASE_P_INDEX_P_DISP,
    EA_BASE_P_INDEXS_P_DISP
};

/* Determine_ea_kind() -- To reduce the number of error introduced
 * when distinguishing one kind of EA calculation from another in
 * the Is_ea_* routines, we have put all of the code in this one place. */
static ea_kinds
Determine_ea_kind(instruction *ea)
{
    immed im;
    operand s0, s1;
    
    switch (ea->opcode()) {	// switch on root operation
      case io_add:
	s0 = ea->src_op(0);
	if (s0.is_instr()) {
	    // one of the base+(index*scale)+disp forms
	    instruction *e = s0.instr();
	    assert(e->opcode() == io_add);
	    s0 = e->src_op(0);
	    if (s0.is_null())
		return EA_INDEXS_P_DISP;

	    e = e->src_op(1).instr();
	    assert(e->opcode() == io_mul);
	    int s = e->src_op(1).immediate().integer();
	    if (s == 0)
		return EA_BASE_P_INDEX_P_DISP;
	    else 
		return EA_BASE_P_INDEXS_P_DISP;
	}

	s1 = ea->src_op(1);
	if (s1.is_immed())
	    return EA_BASE_P_DISP;
	else if (s1.is_instr()) {
	    instruction *in = s1.instr();
	    if (in->opcode() == io_add)
		return EA_INDEXED_SYMADDR;
	}
	break;

      case io_ldc:
	im = ((in_ldc *)ea)->value();
	if (im.is_symbol())
	    return EA_SYMADDR;
	break;

      default:
	break;
    }

    return EA_UNKNOWN;		// error
}

/* Is_ea_base_p_disp() -- return TRUE if effective-address calculation
 * has the form of base+displacement. */
boolean
Is_ea_base_p_disp(instruction *ea)
{
    return (Determine_ea_kind(ea) == EA_BASE_P_DISP);
}

/* Is_ea_symaddr() -- return TRUE if effective-address calculation
 * has the form of a simple relocatable symbol. */
boolean
Is_ea_symaddr(instruction *ea)
{
    return (Determine_ea_kind(ea) == EA_SYMADDR);
}

/* Is_ea_indexed_symaddr() -- return TRUE if effective-address calculation
 * has the form of an indexed relocatable symbol. */
boolean
Is_ea_indexed_symaddr(instruction *ea)
{
    return (Determine_ea_kind(ea) == EA_INDEXED_SYMADDR);
}

/* Is_ea_indexS_p_disp() -- return TRUE if effective-address 
 * calculation has the form of index*scale+displacement. */
boolean
Is_ea_indexS_p_disp(instruction *ea)
{
    return (Determine_ea_kind(ea) == EA_INDEXS_P_DISP);
}

/* Is_ea_base_p_index_p_disp() -- return TRUE if effective-address 
 * calculation has the form of base+index+displacement. */
boolean
Is_ea_base_p_index_p_disp(instruction *ea)
{
    return (Determine_ea_kind(ea) == EA_BASE_P_INDEX_P_DISP);
}

/* Is_ea_base_p_indexS_p_disp() -- return TRUE if effective-address 
 * calculation has the form of base+(index*scale)+displacement. */
boolean
Is_ea_base_p_indexS_p_disp(instruction *ea)
{
    return (Determine_ea_kind(ea) == EA_BASE_P_INDEXS_P_DISP);
}


/* ---------- Access helpers ---------- */

/* Get_ea_symaddr_sym() -- return the relocatable symbol from the
 * effective-address calculation.  Perform some simple sanity checks. */
sym_node *
Get_ea_symaddr_sym(instruction *ea)
{
    assert(Is_ea_symaddr(ea));
    immed im = ((in_ldc *)ea)->value();

#ifdef MACHSUIF_DEBUG
    if ((im.addr()).offset() != 0)
	warning_line(ea, "non-zero offset in ea_symaddr, ignored?");
#endif

    return im.symbol();
}

/* Get_ea_symaddr_off() -- return the offset attached to the relocatable
 * symbol from the effective-address calculation.  Perform some simple
 * sanity checks. */
int
Get_ea_symaddr_off(instruction *ea)
{
    assert(Is_ea_symaddr(ea));
    immed im = ((in_ldc *)ea)->value();
    return im.offset();
}

