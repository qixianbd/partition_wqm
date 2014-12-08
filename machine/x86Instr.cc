/* file "x86Instr.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1996-1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */


#include <suif_copyright.h>
#include <suif1.h>
#include <string.h>
#include "machine_internal.h"


static void x86_print_opcode_size(type_node *, FILE *);
static void x86_print_opcode_type(machine_instr *, FILE *);
static void x86_print_opcode(machine_instr *, FILE *);
static void x86_print_operand(operand *, FILE *);
static void x86_print_ea_s0_p_s1(instruction *, FILE *);
static void x86_print_ea_operand(operand *, FILE *);


/*
 *  The following are helper definitions and classes that are
 *  basically a cheap way of implementing the (unimplemented)
 *  base class x86_instr.
 */

char *k_x86;

const char* leading_underscore = ""; //"_"; // added by duyanning


/* init_x86() -- do all of the priliminary stuff required to
 * use the X86 instruction classes. */
void
init_x86()
{
   /* setup useful string constants */
    k_x86 = lexicon->enter("x86")->sp;
}


/* ---------------- Is_* helper routines ------------------- */

boolean
x86_is_ldc(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == xo_mov) && i->src_op(0).is_immed() && !Writes_memory(i));
}

/* x86_is_move() -- Return true for moves that are (potentially) register-
 * to-register.  Rule out a load, ldc, or store, which have the same opcode
 * as a register move: src_op(0) can't be an effective address or immediate
 * value, and the instruction can't write memory. */
boolean
x86_is_move(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == xo_mov) && !i->src_op(0).is_instr() && !Writes_memory(i));
}

boolean
x86_is_cmove(instruction *i)
{
    int o = (int)i->opcode();
    return FALSE;
}

boolean
x86_is_line(instruction *i)
{
    int o = (int)i->opcode();
    return (o == xo_loc);
}

boolean
x86_is_ubr(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == xo_jmp)
        && (!i->peek_annote(k_instr_ret))
        && (!i->peek_annote(k_instr_mbr_tgts)));
}

boolean
x86_is_cbr(instruction *i)
{
    int o = (int)i->opcode();
    return ((o >= xo_ja) && (o <= xo_loopz));
}

boolean
x86_is_call(instruction *i)
{
    int o = (int)i->opcode();
    return (o == xo_call);
}

boolean
x86_is_return(instruction *i)
{
    int o = (int)i->opcode();
    return (o == xo_ret);
}

boolean
x86_reads_memory(instruction *i)
{
    unsigned j;
    for (j = 0; j < i->num_srcs(); j++) {
	operand o = i->src_op(j);
	if (Is_ea_operand(o)
	&& ((o.instr())->peek_annote(k_is_store_ea) == NULL))
	    return TRUE;
    }

    return FALSE;
}

/* -------------- Code generation helper routines ----------------- */

mi_ops
x86_load_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
      case SIZE_OF_HALFWORD:
        assert(t->op() != TYPE_FLOAT);
        return xo_mov;

      case SIZE_OF_WORD:
        return (t->op() == TYPE_FLOAT) ? xo_fld : xo_mov;

      case SIZE_OF_DOUBLE:
	assert_msg((t->op() == TYPE_FLOAT),
		   ("Load_op() -- can only load 64-bit FP numbers"));
	return xo_fld;

      default:
        t->print_full(stderr);
        assert_msg(FALSE, ("Load_op() -- Unknown load size for io_lod."));
	return xo_null;
    }
}

mi_ops
x86_store_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
      case SIZE_OF_HALFWORD:
        assert(t->op() != TYPE_FLOAT);
        return xo_mov;

      case SIZE_OF_WORD:
        return (t->op() == TYPE_FLOAT) ? xo_fstp : xo_mov;

      case SIZE_OF_DOUBLE:
	assert_msg((t->op() == TYPE_FLOAT),
		   ("Store_op() -- can only store 64-bit FP numbers"));
	return xo_fstp;

      default:
        t->print_full(stderr);
        assert_msg(FALSE, ("Store_op() -- Unknown load size for io_str."));
	return xo_null;
    }
}

mi_ops
x86_move_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
      case SIZE_OF_HALFWORD:
        assert(t->op() != TYPE_FLOAT);
        return xo_mov;

      case SIZE_OF_WORD:
        return (t->op() == TYPE_FLOAT) ? xo_fstp : xo_mov;

      case SIZE_OF_DOUBLE:
	assert_msg((t->op() == TYPE_FLOAT),
		   ("Move_op() -- can only move 64-bit FP numbers"));
	return xo_fld;

      default:
        t->print_full(stderr);
        assert_msg(FALSE, ("Move_op() -- Unexpected size for move."));
	return xo_null;
    }
}

/* ---------------- print helper routines ------------------- */

/* x86_print_opcode_type() -- print the "l", "w", or "b" after an ALU opcode
 * required by the AT&T x86 assembler syntax.   This routine also handles
 * "s" and "l" size specifiers for FP opcodes. */
static void
x86_print_opcode_size(type_node *t, FILE *o_fd)
{
    int size = t->size();

    if (t->op() == TYPE_FLOAT) {
	if (size == SIZE_OF_SINGLE)
	    fprintf(o_fd, "s");
	else if (size == SIZE_OF_DOUBLE) 
	    fprintf(o_fd, "l");
	else
	    assert_msg(FALSE, ("x86_print_opcode_size() -- "
			       "unexpected fp size (%d)", size));

    } else {
	if (size == SIZE_OF_WORD)
	    fprintf(o_fd, "l");
	else if (size == SIZE_OF_HALFWORD)
	    fprintf(o_fd, "w");
	else if (size == SIZE_OF_BYTE)
	    fprintf(o_fd, "b");
	else
	    assert_msg(FALSE, ("x86_print_opcode_size() -- "
			       "unexpected int size (%d)", size));
    }
}

static void
x86_print_opcode_type(machine_instr *i, FILE *o_fd)
{
    mi_ops opcode = i->opcode();

    if (((int)i->format() != (int)mif_rr)
	|| (opcode == xo_sete)  || (opcode == xo_setl) 
	|| (opcode == xo_setle) || (opcode == xo_setne) || (opcode == xo_cdq)
	|| (opcode == xo_cbw)   || (opcode == xo_movsd) 
	|| (opcode == xo_movsw) || (opcode == xo_movsb) || (opcode == xo_cld)
	|| (opcode == xo_leave) || (opcode == xo_ret))
	return;

    if (opcode == xo_mov) {
	if (Writes_memory(i))
	    x86_print_opcode_size(i->src_op(1).type(), o_fd);
	else {
	    // Reads_memory(i) or simple reg-reg move
	    x86_print_opcode_size(i->dst_op().type(), o_fd);
	}

    } else if ((opcode == xo_movsx) || (opcode == xo_movzx)) {
	/* These opcodes need a size for the source and a size 
	 * for the destination. */
	if (Reads_memory(i)) {
	    // unwrap ptr type for source effective address
	    ptr_type *t_s0 = (ptr_type *)i->src_op(0).type();
	    x86_print_opcode_size(t_s0->ref_type(), o_fd);
	} else {
	    // movsx and movzx do not ever write memory
	    x86_print_opcode_size(i->src_op(0).type(), o_fd);
	}
	x86_print_opcode_size(i->dst_op().type(), o_fd);

    } else if ((opcode == xo_fild) || (opcode == xo_fist) ||
	       (opcode == xo_fistp)) {
	// unwrap ptr type for memory effective address
	ptr_type *t_s0 = (ptr_type *)i->src_op(0).type();
	x86_print_opcode_size(t_s0->ref_type(), o_fd);

    } else if (opcode == xo_cmp || opcode == xo_test) {
	x86_print_opcode_size(i->src_op(0).type(), o_fd);

    } else if (i->result_type() == type_void) {
	if (Reads_memory(i)) {
	    // unwrap ptr type for source effective address
	    ptr_type *t_s0 = (ptr_type *)i->src_op(0).type();
	    x86_print_opcode_size(t_s0->ref_type(), o_fd);
	} else {
	    x86_print_opcode_size(i->src_op(0).type(), o_fd);
	}

    } else if (!i->src_op(0).is_null()) {
	x86_print_opcode_size(i->result_type(), o_fd);
    }
}

/* x86_print_opcode() -- prints out the machine opcode and any extensions
 * in x86-specific syntax. */
static void
x86_print_opcode(machine_instr *i, FILE *o_fd)
{
    immed_list *iml;

    // prefix bytes (aka opcode extensions) go first in x86
    if ((iml = (immed_list *)i->peek_annote(k_instr_op_exts))) {
	immed_list_iter ili(iml);
	while (!ili.is_empty()) {
	    immed im =  ili.step();
	    fprintf(o_fd, "\t%s", x86_op_ext_string(im.integer()));
	}
    }

    fprintf(o_fd, "\t%s", i->op_string());

    x86_print_opcode_type(i, o_fd);
    fprintf(o_fd, "\t");
}


/* x86_print_ea_s0_p_s1() -- helper routine for x86_print_ea_operand(). */
static void
x86_print_ea_s0_p_s1(instruction *in, FILE *o_fd)
{
    assert(in->opcode() == io_add);
    operand in_s0 = in->src_op(0);
    operand in_s1 = in->src_op(1);

    /* print first operand */
    x86_print_operand(&in_s0, o_fd);

    /* Print second operand after first determining if we need a '+'
     * or not.  Do not print anything more if 2nd operand is zero.
     * In this assembler syntax, we do not print a '$' before an
     * literal offset. */
    if (in_s1.is_immed()) {
	immed in_s1_im = in_s1.immediate();
	if (in_s1_im.is_integer()) {
	    if (in_s1_im.integer() > 0)
		fprintf(o_fd, "+");
	    if (in_s1_im.integer() != 0)
		fprintf(o_fd, "%d", in_s1_im.integer());

	} else if (in_s1_im.is_ext_integer()) {
	    if ((in_s1_im.ext_integer())[0] != '-')
		fprintf(o_fd, "+");
	    fprintf(o_fd, "%s", in_s1_im.ext_integer());

	} else {
	    assert("x86_print_ea_s0_p_s1() -- "
		   "unexpected immed type in instruction");
	}

    } else {
	fprintf(o_fd, "+");
	x86_print_operand(&in_s1, o_fd);
    }
}

/* x86_print_ea_operand() -- prints out an effective address operand
 * in x86-specific syntax. */
static void
x86_print_ea_operand(operand *o, FILE *o_fd)
{
    assert(Is_ea_operand(*o));

    instruction *ea = o->instr();
    if_ops op = ea->opcode();

    if (Is_ea_base_p_disp(ea)) {
	operand ea_s0 = ea->src_op(0);
	if (ea_s0.is_reg()) {
	    if (ea_s0.reg() == REG_const0) {
		// Special case since x86 doesn't have a const0 register.
		// Just print the displacement.
		operand ea_s1 = ea->src_op(1);
		x86_print_operand(&ea_s1, o_fd);
	    } else {
		/* Print as 'offset(base_reg)' -- offset already in bytes.
		 * Don't use x86_print_operand because we don't want 
		 * a '$' in front of the offset.  As a point of note,
		 * x86 offsets are negative from EBP. */
		fprintf(o_fd, "%d", ea->src_op(1).immediate().integer());
		fprintf(o_fd,"(");
		x86_print_operand(&ea_s0, o_fd);
		fprintf(o_fd,")");
	    }

	} else {
	    /* print as 's0+s1' */
	    x86_print_ea_s0_p_s1(ea, o_fd);
	}

    } else if (Is_ea_symaddr(ea)) {
	/* print as 'symbol+offset' -- offset of symaddr is unsigned */
	immed ea_im = ((in_ldc *)ea)->value();
	assert(ea_im.is_symbol());
	fprintf(o_fd, leading_underscore);
	Print_symbol(o_fd, ea_im.symbol());
	if (ea_im.offset()) {
	    fprintf(o_fd, "+%d", ea_im.offset() >> SHIFT_BITS_TO_BYTES);
	}

    } else if (Is_ea_indexS_p_disp(ea) || Is_ea_base_p_index_p_disp(ea)
	       || Is_ea_base_p_indexS_p_disp(ea)) {
	// All these forms are printed as 'disp(base_reg,index_reg,scale)'.
	// Both the scale factor and the displacement are already in bytes.
	// The printing of scale and disp do not use x86_print_operand
	// because we don't want a '$' in front of either one.
	instruction *e;
	operand e_s0;
	fprintf(o_fd, "%d(",
		ea->src_op(1).immediate().integer());	// print disp
	e = ea->src_op(0).instr();
	e_s0 = e->src_op(0);
	if (!e_s0.is_null())
	    x86_print_operand(&e_s0, o_fd);		// print base_reg
	fprintf(o_fd,",");
	e = e->src_op(1).instr();
	e_s0 = e->src_op(0);
	x86_print_operand(&e_s0, o_fd);			// print index_reg
	fprintf(o_fd, ",%d)",
		e->src_op(1).immediate().integer());	// print scale

    } else
	assert_msg(FALSE, ("x86_print_ea_operand() -- unexpected "
			   "ea kind (opcode = %s)", if_ops_name(op)));
}

/* x86_print_operand() -- prints out an operand in x86-specific syntax. */
static void
x86_print_operand(operand *o, FILE *o_fd)
{
    int r;

    switch (o->kind()) {
      case OPER_NULL:
	break;

      case OPER_SYM:
	fprintf(o_fd, leading_underscore);
	Print_symbol(o_fd, o->symbol());
	break;

      case OPER_REG:
	r = o->reg();
	if (r < 0)				/* virtual register */
	    fprintf(o_fd, "$vr%d", -r);

        else {                                  /* hard register */
	    int sz = o->type()->size();
	    reg_desc d = target_regs->describe(r);
	    if (d.bank == GPR) {
		if ((d.conv == TMP) && (d.encoding <= 3)) {
		    // EAX, EBX, ECX, EDX
		    if (sz == SIZE_OF_WORD)
			fprintf(o_fd, "%%e%sx", d.name);
		    else if (sz == SIZE_OF_HALFWORD)
			fprintf(o_fd, "%%%sx", d.name);
		    else // SIZE_OF_BYTE
			fprintf(o_fd, "%%%sl", d.name);
		} else {
		    // ESI, EDI, ESP, EBP
		    if (sz == SIZE_OF_WORD)
			fprintf(o_fd, "%%e%s", d.name);
		    else // SIZE_OF_HALFWORD
			fprintf(o_fd, "%%%s", d.name);
		}
	    } else {
		// segment and control registers
		fprintf(o_fd, "%%%s", d.name);
	    }
	}
	break;

      case OPER_INSTR:
	  if (Is_ea_operand(*o))	// an eff. addr. operand
	    x86_print_ea_operand(o, o_fd);

	else {				// an immediate operand
	    immed o_im = o->immediate();
	    if (o_im.is_integer())
		fprintf(o_fd, "$%d", o_im.integer());
	    else if (o_im.is_ext_integer())
		fprintf(o_fd, "$%s", o_im.ext_integer());
	    else if (o_im.is_symbol()) {
		fprintf(o_fd, "$");
		Print_symbol(o_fd, o_im.symbol());
	    } else
		assert_msg(FALSE, ("x86_print_operand() -- "
				   "unexpected kind of immediate operand"));
	}
	break;

      default:
	assert_msg(FALSE,
	    ("x86_print_operand() -- unknown kind %d", o->kind()));
    }
}

/*
 *  Methods in public "x86_instr" classes.
 */

/* x86_lab_print() -- print out label in x86 format. */
void
x86_lab_print(mi_lab *l, FILE *o_fd)
{
    fprintf(o_fd,leading_underscore);
    l->label()->sym_node::print(o_fd);
    fprintf(o_fd,":");                  /* x86-specific ending */
    l->print_comment(o_fd, "#");
}

/* x86_bj_print() -- print out instruction of mif_bj format. */
void
x86_bj_print(mi_bj *bj, FILE *o_fd)
{
    /* print out opcode and any extensions */
    x86_print_opcode(bj, o_fd);

    /* print out target address */
    if (bj->target() != NULL) {
	fprintf(o_fd, leading_underscore); // x86 - precede procedure names with an underscore
	Print_symbol(o_fd, bj->target());
    } else {
	operand bj_s0 = bj->src_op(0);
	x86_print_operand(&bj_s0, o_fd);
    }

    bj->print_comment(o_fd, "#");
}


/* x86_rr_print() -- print out instruction of mif_rr format.  x86
 * instructions that print out their operands in a weird order are
 * handled here. */
void
x86_rr_print(mi_rr *r, FILE *o_fd)
{
    /* print out opcode and any extensions */
    x86_print_opcode(r, o_fd);

    /* print out any operands -- some opcodes require special printing 
     * format for the operands */
    int o = r->opcode();

    if ((o == xo_push) || (o == xo_idiv) || (o == xo_div) ||
// Check this next statement
	(r->result_type()->op() == TYPE_FLOAT)) {
	// Push's, divides, ??? and print s0
	operand r_s1 = r->src_op(1);
	x86_print_operand(&r_s1, o_fd);

    } else if ((o == xo_cbw) || (o == xo_cwd) || (o == xo_cdq) || 
	(o == xo_ret) || (o == xo_movsb) || (o == xo_movsw) ||
	(o == xo_movsd)) {
	// special cases - do not print any operands

    } else if (Writes_memory(r)) {	// is store
	// print out register source
	assert(r->dst_op().is_null());
	if ((o == xo_sete) || (o == xo_setl) || (o == xo_setle) || 
	    (o == xo_setne)) {
	    // implicit source of data to write
	} else {
	    assert((r->num_srcs() > 1) && !r->src_op(1).is_null());
	    operand r_s1 = r->src_op(1);
	    x86_print_operand(&r_s1, o_fd);
	    fprintf(o_fd, ",");
	}

	// print out effective address operand--in srcs[0] by convention
	operand r_s0 = r->src_op(0);
	x86_print_operand(&r_s0, o_fd);

    } else if (o == xo_cmp) {
	// in AT&T syntax, s1 comes before (left of) s0
	operand r_s1 = r->src_op(1);
	x86_print_operand(&r_s1, o_fd);
	fprintf(o_fd, ",");
	operand r_s0 = r->src_op(0);
	x86_print_operand(&r_s0, o_fd);

    } else if (o == xo_mov) {
	operand s = r->src_op(0), d = r->dst_op();
	if (s.is_hard_reg() && (s.type())->size() != (d.type())->size()) {
	    /* hack to print source operand not according to type */
	    operand z(s.reg(), d.type());
	    x86_print_operand(&z, o_fd);
	} else {
	    operand r_s0 = r->src_op(0);
	    x86_print_operand(&r_s0, o_fd);
	}
	fprintf(o_fd, ",");
	operand rd = r->dst_op();
	x86_print_operand(&rd, o_fd);

    } else if (Reads_memory(r) || o == xo_movsx || o == xo_movzx) {
	operand r_s0 = r->src_op(0);
	x86_print_operand(&r_s0, o_fd);
	fprintf(o_fd, ",");
	operand rd = r->dst_op();
	x86_print_operand(&rd, o_fd);

    } else if (r->src_op(1).is_null()) {
	// general single-source-operand case
	if (!r->dst_op().is_null()) {
	    // s0 == d0
	    operand rd = r->dst_op();
	    x86_print_operand(&rd, o_fd);
	} else if (!r->src_op(0).is_null()) {
	    // s0 but no d0
	    operand r_s0 = r->src_op(0);
	    x86_print_operand(&r_s0, o_fd);
	} // else print nothing after opcode

    } else {
	// general 2-source-operand case
	// print out "src_op(1), dst_op(0)" in that order
	assert_msg(!r->dst_op().is_null(),
		   ("x86_rr_print() -- missed case??? "
		    "s1 valid but dst null"));
	operand r_s1 = r->src_op(1);
	x86_print_operand(&r_s1, o_fd);
	fputc(',', o_fd);
	operand rd = r->dst_op();
	x86_print_operand(&rd, o_fd);
    }

    r->print_comment(o_fd, "#");
}

/* x86_xx_print() -- print out instruction of mif_xx format */
void
x86_xx_print(mi_xx *x, immed_list *il, FILE *o_fd)
{
    immed_list_iter ili(il);

    /* Special case xo_null so we don't output null lines */
    if (x->opcode() == (int)xo_null) {
	if (x->are_annotations()) x->print_comment(o_fd, "#");
	return;
    }

    /* print out opcode */
    fprintf(o_fd, "\t%s\t", x->op_string());

    switch (x->opcode()) {
      case xo_file:				/* .file "<file_name>" */
	assert(il->count() == 2);
	ili.step().integer();				/* file no. */
	fprintf(o_fd,"\"%s\"", ili.step().string());	/* file name */
	break;

      case xo_globl:
	fprintf(o_fd, leading_underscore);
	Print_raw_immed(o_fd, ili.step());
	break;

      case xo_ascii:    /* .ascii* ["<s1>"[, "<s2>"...]] */
        while (!ili.is_empty()) {
            /* while >1 element in list, print with comma */
            fprintf(o_fd,"\"%s\"", ili.step().string());
            if (!ili.is_empty()) fprintf(o_fd,", ");
        }
        break;

      /* Print arguments with commas and strings without quotes */
      default:
	while (!ili.is_empty()) {
	    /* while >1 element in list, print with comma */
	    immed i = ili.step();
	    if (i.is_symbol())
		fprintf(o_fd, leading_underscore);
	    Print_raw_immed(o_fd, i);
	    if (!ili.is_empty()) fprintf(o_fd,", ");
	}
	break;

    }

    x->print_comment(o_fd, "#");
}


/*
 *  Other helpful print methods that are architecture-specific.
 */

/* x86_print_global_directives() -- */
void
x86_print_global_directives(file_set_entry *fse, FILE *o_fd)
{
    /* do nothing */
}


/* x86_print_extern_op() -- */
void
x86_print_extern_op(var_sym *v, FILE *o_fd)
{
    /* do nothing */
}


/* x86_print_file_op() -- */
void
x86_print_file_op(int fnum, char *fnam, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(xo_file, immed(fnum));
    mi->append_operand(immed(fnam));
    mi->print(o_fd);
    delete mi;
}


/* x86_print_var_def() -- Generates assembly language data statements.
 * Data objects are put into the data section, comm section, or the lcomm 
 * section.  If the data object is global, I also specify a .globl pseudo-op.
 *
 * There is a special case though to cover the problem of the front end
 * creating a big array filled with zeros.  If the array is big enough
 * and I tell the assembler to initialize it, the assembler will die.
 * Consequently, I check to see if the data item has only a single
 * annotation, the annotation fills with zeros, and the item is bigger
 * than 1024 bytes in size.  If so, the data item is made a .comm and
 * I rely on UNIX to initialize it to zeros at link time.  If the
 * data item is bigger than 64K bytes, I print a warning that the assembler
 * may die. */
void
x86_print_var_def(var_sym *vsym, FILE *o_fd)
{
    mi_xx *mi;
    annote *an;
    annote_list *anl;
    immed_list *il;

    assert(vsym->has_var_def());	/* sanity check */
    var_def *vdef = vsym->definition();

    /* Check for big zero arrays -- if found, remove the initialization
     * annotation so that the initialization is done automatically. */
    if (((vsym->type()->size() >> SHIFT_BITS_TO_BYTES) > 1024)
    && (vdef->are_annotations() && vdef->annotes()->count() == 1)) {
	anl = vdef->annotes();
	if ((an = anl->get_annote(k_repeat_init))) {
	    /* check if repeat value is 0 or 0.0 */
	    immed i = (*(an->immeds()))[2];	/* repeat value */
	    if (i.is_integer()) {
		if (i.integer()) anl->append(an);	/* test failed */
	    } else if (i.is_flt()) {
		if (i.flt() != 0.0) anl->append(an);	/* test failed */
	    } else {
		anl->push(an);				/* test failed */
	    }

	} else if ((an = anl->get_annote(k_fill))) {
	    /* check if fill pattern is 0x0 */
	    immed i = (*(an->immeds()))[1];	/* fill pattern */
	    if (!i.is_integer() || (i.integer() != 0))
		anl->push(an);				/* test failed */
	    
	}					/* failed limited tests */

    } else if (((vsym->type()->size() >> SHIFT_BITS_TO_BYTES) > 65536)
	       && vdef->are_annotations())
	warning_line(NULL,
	    "Initialized data item %s may be too big for assembler.",
	    vsym->name());

    if (vdef->are_annotations()) {		/* initialized data item */
	/* On x86, always use .data section. */
	mi = new mi_xx(xo_data);
	mi->print(o_fd); delete mi;

	/* If global symbol, add .globl pseudo-op.  Note that a variable
	 * is considered global in SUIF if it is part of the global or
	 * file symbol table.  For x86 assembly, a datum is global only
	 * if it is part of the global symbol table (i.e. is_private()
	 * checks for def in file symbol table). */
	if (vsym->is_global() && !vsym->is_private()) {
	    mi = new mi_xx(xo_globl,immed(vsym));
	    mi->print(o_fd); delete mi;
	}

	/* Determine alignment and align */
	mi = new mi_xx(xo_align);
	switch (vdef->alignment()) {
	  case SIZE_OF_BYTE:			/* byte aligned */
	    mi->append_operand(immed(0));
	    break;
	  case SIZE_OF_HALFWORD:		/* halfword aligned */
	    mi->append_operand(immed(1));
	    break;
	  case SIZE_OF_WORD:			/* word aligned */
	    mi->append_operand(immed(2));
	    break;
	  case SIZE_OF_DOUBLE:			/* double word aligned */
	    mi->append_operand(immed(3));
	    break;
	  case SIZE_OF_QUAD:			/* quad word aligned */
	    mi->append_operand(immed(4));
	    break;
	  default:
	    assert_msg(FALSE,
		("x86_print_var_def() -- bad alignment for %s", vsym->name()));
	}
	mi->print(o_fd); delete mi;

	/* Disable automatic alignment */
	mi = new mi_xx(xo_align,immed(0));
	il = new immed_list();
	il->append(immed("disable automatic alignment"));
	mi->append_comment(il);
	mi->print(o_fd); delete mi;

	/* Write out the label */
	fprintf(o_fd, leading_underscore);
	Print_data_label(o_fd, vsym, ':');

	/* Initialize memory with values -- do not destroy data
	 * annotations in the process. */
	annote_list_iter an_iter(vdef->annotes());
	while (!an_iter.is_empty()) {
	    an = an_iter.step();
	    il = an->immeds();

	    /* new-SUIF deprecated old-SUIF annotation "init" */

	    if (an->name() == k_multi_init) {
		char data_string[64];
		int values = il->count();	/* includes value_size entry */
		int value_size = (*il)[0].integer();

		if (value_size == SIZE_OF_BYTE) {
		    int v_i, ds_i;

		    /* Create one or more pseudo-ops.  The immediate values
		     * will be built into strings of length 64, at most.  If
		     * values > 64, we'll create multiple strings. */
		    ds_i = 0;
		    for (v_i = 1; v_i < values; v_i++) {
			int c = (*il)[v_i].integer();

			/* Need to start new string?  Remember to leave
			 * enough space for a hex value (4) plus the
			 * ending null. */
			if (ds_i > 59) {
			    data_string[ds_i] = '\0';	// end current str
			    mi = new mi_xx(xo_ascii,immed(data_string));
			    mi->print(o_fd);
			    delete mi;
			    ds_i = 0;			// reset for new str
			}

			/* insert value into current string as char */
			if (c == (int)'\\') {
			    data_string[ds_i++] = '\\';
			    data_string[ds_i++] = '\\';
			} else if ((c >= (int)' ') && (c != (int)'"')
				   && (c <= (int)'~')) {
			    data_string[ds_i++] = (char)c;
			} else {
			    char octal_value[4];
			    data_string[ds_i++] = '\\';
			    sprintf(octal_value,"%03o",(unsigned char)c);
			    data_string[ds_i++] = octal_value[0];
			    data_string[ds_i++] = octal_value[1];
			    data_string[ds_i++] = octal_value[2];
			}
		    }

		    /* end current string */
		    data_string[ds_i] = '\0';
		    mi = new mi_xx(xo_ascii,immed(data_string));
		    mi->print(o_fd);
		    delete mi;

		} else 
		    assert_msg(FALSE,
			("x86_print_var_def() -- k_multi-init value size = %d",
			 value_size));

	    } else if (an->name() == k_repeat_init) {
		int repeat_cnt = (*il)[0].integer();
		int value_size = (*il)[1].integer();
		immed ivalue = (*il)[2];

		if (repeat_cnt > 1) {	/* insert .repeat pseudo-op */
		    mi = new mi_xx(xo_repeat,immed(repeat_cnt));
		    mi->print(o_fd); delete mi;
		}

		/* actual data op */
		switch (value_size) {		/* to get value size */
		  case SIZE_OF_BYTE:
		    mi = new mi_xx(xo_byte, ivalue);
		    break;
		  case SIZE_OF_HALFWORD:
		    mi = new mi_xx(xo_word, ivalue);
		    break;
		  case SIZE_OF_WORD:
		    if (ivalue.is_integer() || ivalue.is_ext_integer())
			mi = new mi_xx(xo_long, ivalue);
		    else if (ivalue.is_flt())		/* got a FP single */
			mi = new mi_xx(xo_float, ivalue);
		    else if (ivalue.is_symbol())
			mi = new mi_xx(xo_long, ivalue);
		    else
			assert_msg(FALSE,
			    ("x86_print_var_def() -- "
			     "bad k_repeat_init value"));
		    break;
		  case SIZE_OF_DOUBLE:
		    assert(ivalue.is_flt());
		    mi = new mi_xx(xo_double, ivalue);
		    break;
		  default:
		    assert_msg(FALSE,
		        ("x86_print_var_def() -- bad size for k_repeat_init"));
		}
		mi->print(o_fd); delete mi;

		if (repeat_cnt > 1) {	/* insert .endr pseudo-op */
		    mi = new mi_xx(xo_endr);
		    mi->print(o_fd); delete mi;
		}

	    } else if (an->name() == k_fill) {
		/* fill size with bit pattern, assuming size is a multiple
		 * of bytes and the bit pattern is all zeros. */
		int value_size = (*il)[0].integer();
		assert((value_size & 7) == 0);
		assert((*il)[1].integer() == 0);

		/* insert .repeat pseudo-op */
		mi = new mi_xx(xo_repeat,
			       immed(value_size >> SHIFT_BITS_TO_BYTES));
		mi->print(o_fd); delete mi;

		/* insert .byte 0 */
		mi = new mi_xx(xo_byte,(*il)[1]);
		mi->print(o_fd); delete mi;

		/* insert .endr pseudo-op */
		mi = new mi_xx(xo_endr);
		mi->print(o_fd); delete mi;

	    } else
	        warning_line(NULL, "x86_print_var_def() -- %s (%s)"
		        "ignoring unknown var_def annote", an->name());
	}

    } else {				/* uninitialized data item */
	/* if global symbol, append to .comm, else static symbol to .lcomm */
	if (vsym->is_global() && !vsym->is_private())
	    mi = new mi_xx(xo_comm, immed(vsym));
	else
	    mi = new mi_xx(xo_lcomm, immed(vsym));
	mi->append_operand(immed(vsym->type()->size() >> SHIFT_BITS_TO_BYTES));
	mi->print(o_fd); delete mi;
   }
}


/* x86_print_proc_def() -- */
void
x86_print_proc_def(proc_sym *p, FILE *o_fd)
{
    /* do nothing */
}


/* x86_print_proc_begin() -- Starts a procedure text segment. */
void
x86_print_proc_begin(proc_sym *psym, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(xo_text);				/* .text */
    mi->print(o_fd);
    delete mi;
    mi = new mi_xx(xo_align,immed(2));				/* .align 2 */
    mi->print(o_fd);
    delete mi;
}


/* x86_print_proc_entry() -- Reads the symbol table information on
 * the proc_sym and the annotations off the tree_proc.  It creates the
 * correct x86 instructions to start a procedure text segment. */
void
x86_print_proc_entry(proc_sym *psym, int file_no_for_1st_line, FILE *o_fd)
{
    mi_xx *mi;
    mi = new mi_xx(xo_align,immed(2));                          /* .align 2 */
    mi->print(o_fd);
    delete mi;

    /* if global procedure, then add this pseudo-op.  I.e. procedure is
     * in global symbol table, not in the file symbol table. */
    if (psym->is_global() && !psym->is_private()) {
	mi = new mi_xx(xo_globl,immed(psym->name()));		/* .globl */
	mi->print(o_fd);
	delete mi;
    }

    /* get file and line no. info for this procedure -- .file annote
     * already generated (if necessary) in Process_file() */
    immed_list *iml = (immed_list *)psym->block()->peek_annote(k_line);
    assert(iml && (strcmp((*iml)[1].string(),"") != 0));
    int first_line_no = (*iml)[0].integer();
    mi = new mi_xx(xo_loc,immed(file_no_for_1st_line));		/* .loc */
    mi->append_operand(immed(first_line_no));
    mi->print(o_fd);
    delete mi;

    /* the procedure symbol cannot be a label symbol since SUIF does not
     * allow label symbols outside of a procedural context (this is the
     * reason why this routine exists in the first place), and so, we
     * have to generate the procedure label in a funny way. */
    //    fprintf(o_fd, "_%s:\n", psym->name());			/* proc: */
    fprintf(o_fd, "%s%s:\n", leading_underscore, psym->name());			/* proc: */ // duyanning

    /* rest of text in tree_block */
}


/* x86_print_proc_end() -- */
void
x86_print_proc_end(proc_sym *psym, FILE *o_fd)
{
    // do nothing
}
