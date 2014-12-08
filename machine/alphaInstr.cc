/* file "alphaInstr.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,96 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>
#include <suif1.h>
#include <string.h>
#include "machine_internal.h"


static void alpha_print_opcode(machine_instr *, FILE *);
/*static*/ void alpha_print_operand(operand *, FILE *);
static void alpha_print_ea_s0_p_s1(instruction *, FILE *);
static void alpha_print_ea_operand(operand *, FILE *);
static void alpha_print_reloc(machine_instr *, FILE *);


/*
 *  The following are helper definitions and classes that are
 *  basically a cheap way of implementing the (unimplemented)
 *  base class alpha_instr.
 */

char *k_alpha;
char *k_gprel_init;


/* init_alpha() -- do all of the priliminary stuff required to
 * use the ALPHA instruction classes. */
void
init_alpha()
{
    /* setup useful string constants */
    k_alpha = lexicon->enter("alpha")->sp;

    ANNOTE(k_gprel_init, "gprel_init", TRUE);
}


/* ---------------- Is_* helper routines ------------------- */

boolean
alpha_is_ldc(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == ao_ldil) || (o == ao_ldiq)
	|| ((o >= ao_ldif) && (o <= ao_ldit))); 
}

boolean
alpha_is_move(instruction *i)
{
    int o = (int)i->opcode();
    if ((o == ao_mov) || (o == ao_fmov))
	return TRUE;
    else
	return FALSE;
}

boolean
alpha_is_cmove(instruction *i)
{
    int o = (int)i->opcode();
    if (((o >= ao_cmoveq) && (o <= ao_cmovlbs))
	|| ((o >= ao_fcmoveq) && (o <= ao_cmovge)))
	return TRUE;
    else
	return FALSE;
}

boolean
alpha_is_line(instruction *i)
{
    int o = (int)i->opcode();
    if (o == ao_loc)
	return TRUE;
    else
	return FALSE;
}

boolean
alpha_is_ubr(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == ao_jmp) || (o == ao_br))
	&& (!i->peek_annote(k_instr_ret))
	&& (!i->peek_annote(k_instr_mbr_tgts)); 
}

boolean
alpha_is_cbr(instruction *i)
{
    int o = (int)i->opcode();
    return (((o >= ao_beq) && (o <= ao_bne_T))
	|| ((o >= ao_fbeq) && (o <= ao_fbge))); 
}

boolean
alpha_is_call(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == ao_bsr) || (o == ao_jsr) || (o == ao_jsr_coroutine)); 
}

boolean
alpha_is_return(instruction *i)
{
    int o = (int)i->opcode();
    return (o == ao_ret); 
}

boolean
alpha_reads_memory(instruction *i)
{
    int o = (int)i->opcode();
    return (((o >= ao_ldb) && (o <= ao_uldq))
	|| ((o >= ao_ldf) && (o <= ao_ldt))); 
}


/* -------------- Code generation helper routines ----------------- */

mi_ops
alpha_load_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
	assert(t->op() != TYPE_FLOAT);
	if (t->is_base())
	    return ((base_type *)t)->is_signed() ? ao_ldb : ao_ldbu;
	else
	    return ao_ldbu;

      case SIZE_OF_HALFWORD:
	assert(t->op() != TYPE_FLOAT);
	if (t->is_base())
	    return ((base_type *)t)->is_signed() ? ao_ldw : ao_ldwu;
	else
	    return ao_ldwu;

      case SIZE_OF_WORD:
	return (t->op() == TYPE_FLOAT) ? ao_lds : ao_ldl;

      case SIZE_OF_DOUBLE:
	return (t->op() == TYPE_FLOAT) ? ao_ldt : ao_ldq;

      default:
	t->print_full(stderr);
	assert_msg(FALSE, ("Load_op() -- Unknown load size for io_lod."));
	return ao_null;
    }
}

mi_ops
alpha_store_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
	assert(t->op() != TYPE_FLOAT);
	return ao_stb;

      case SIZE_OF_HALFWORD:
	assert(t->op() != TYPE_FLOAT);
	return ao_stw;

      case SIZE_OF_WORD:
	return (t->op() == TYPE_FLOAT) ? ao_sts : ao_stl;

      case SIZE_OF_DOUBLE:
	return (t->op() == TYPE_FLOAT) ? ao_stt : ao_stq;

      default:
	t->print_full(stderr);
	assert_msg(FALSE, ("Store_op() -- Unknown load size for io_str."));
	return ao_null;
    }
}

mi_ops
alpha_move_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
      case SIZE_OF_HALFWORD:
	assert(t->op() != TYPE_FLOAT);
	return ao_mov;

      case SIZE_OF_WORD:
      case SIZE_OF_DOUBLE:
	return (t->op() == TYPE_FLOAT) ? ao_fmov : ao_mov;

      default:
	t->print_full(stderr);
	assert_msg(FALSE, ("Move_op() -- Unexpected size for move."));
	return ao_null;
    }
}

/* ---------------- print helper routines ------------------- */

/* alpha_print_opcode() -- prints out the machine opcode and any extensions
 * in ALPHA-specific syntax. */
static void
alpha_print_opcode(machine_instr *i, FILE *o_fd)
{
    immed_list *iml;
    fprintf(o_fd, "\t%s", i->op_string());
    if ((iml = (immed_list *)i->peek_annote(k_instr_op_exts))) {
	immed_list_iter ili(iml);
	while (!ili.is_empty()) {
	    immed im =  ili.step();
	    fprintf(o_fd, "%s", alpha_op_ext_string(im.integer()));
	    /* normally, we'd print the extension separator each time
	     * too, but Alpha has none */
	}
    }
    fprintf(o_fd, "\t");
}


/* alpha_print_ea_s0_p_s1() -- helper routine for alpha_print_ea_operand(). */
static void
alpha_print_ea_s0_p_s1(instruction *in, FILE *o_fd)
{
    assert(in->opcode() == io_add);
    operand in_s0 = in->src_op(0);
    operand in_s1 = in->src_op(1);

    /* print first operand */
    alpha_print_operand(&in_s0, o_fd);

    /* determine if we need a '+' or not */
    boolean opd2_is_0 = FALSE;
    if (in_s1.is_immed()) {
	immed in_s1_im = in_s1.immediate();
	if (in_s1_im.is_integer()) {
	    if (in_s1_im.integer() > 0) fprintf(o_fd, "+");
	    if (in_s1_im.integer() == 0) opd2_is_0 = TRUE;
	} else if (in_s1_im.is_ext_integer()) {
	    if ((in_s1_im.ext_integer())[0] != '-') fprintf(o_fd, "+");
	} else {
	    assert("alpha_print_ea_s0_p_s1() -- "
		   "unexpected immed type in instruction");
	}
    } else
	fprintf(o_fd, "+");

    /* print second operand, if non-zero */
    if (!opd2_is_0)
	alpha_print_operand(&in_s1, o_fd);
}

/* alpha_print_ea_operand() -- prints out an effective address operand
 * in ALPHA-specific syntax. This routine expects the EA operand as
 * an expression, but it can handle flat lists of machine instructions,
 * i.e. lists with SUIF and machine instructions. */
static void
alpha_print_ea_operand(operand *o, FILE *o_fd)
{
    assert(o->is_expr() || o->is_instr());
    instruction *ea = o->instr();
    if_ops op = ea->opcode();

    if (Is_ea_base_p_disp(ea)) {
	operand ea_s0 = ea->src_op(0);
	if (ea_s0.is_reg()) {
	    /* print as 'offset(base_reg)' -- offset already in bytes */
            operand ea_s1 = ea->src_op(1);
	    alpha_print_operand(&ea_s1, o_fd);
	    fprintf(o_fd,"(");
	    alpha_print_operand(&ea_s0, o_fd);
	    fprintf(o_fd,")");

	} else {
	    /* print as 's0+s1' */
	    alpha_print_ea_s0_p_s1(ea, o_fd);
	}

    } else if (Is_ea_symaddr(ea)) {
	/* print as 'symbol+offset' -- offset of symaddr is unsigned */
	immed ea_im = ((in_ldc *)ea)->value();
	Print_symbol(o_fd, ea_im.symbol());
	if (ea_im.offset()) {
	    fprintf(o_fd, "+%d", ea_im.offset() >> SHIFT_BITS_TO_BYTES);
	}

    } else if (Is_ea_indexed_symaddr(ea)) {
	/* print as 'sym+disp($base_reg)' -- offset already in bytes */
	operand ea_s0 = ea->src_op(0);
	assert(ea_s0.is_reg());
	operand ea_s1 = ea->src_op(1);
	assert(ea_s1.is_instr());
	
	/* print subexpression first */
	alpha_print_ea_s0_p_s1(ea_s1.instr(), o_fd);

	/* print index register */
	fprintf(o_fd,"(");
	alpha_print_operand(&ea_s0, o_fd);
	fprintf(o_fd,")");

    } else {
	assert_msg(FALSE, ("alpha_print_ea_operand() -- "
			   "unexpected ea kind (opcode = %s)",
			   if_ops_name(op)));
    }
}


/* alpha_print_operand() -- prints out an operand in ALPHA-specific
 * syntax. */
/*static*/ void
alpha_print_operand(operand *o, FILE *o_fd)
{
    int r;

    switch (o->kind()) {
      case OPER_NULL:
	break;

      case OPER_SYM:
	Print_symbol(o_fd, o->symbol());
	break;

      case OPER_REG:
	r = o->reg();
	if (r < 0)				/* virtual register */
	    fprintf(o_fd, "$vr%d", -r);
	else					/* hard register */
	    fprintf(o_fd, "$%s", target_regs->name(r));
	break;

      case OPER_INSTR:
	/* operand can be an effective address or an immediate */
	if (Is_ea_operand(*o)) alpha_print_ea_operand(o, o_fd);
	else Print_raw_immed(o_fd, o->immediate());
	break;

      default:
	assert_msg(FALSE,
	    ("alpha_print_operand() -- unknown kind %d", o->kind()));
    }
}


/* alpha_print_reloc() -- prints out the relocation information in
 * Alpha-specific syntax.  The last value in the immediate list is
 * the index of the source operand to which this relocation refers.
 * It is not printed. */
static void 
alpha_print_reloc(machine_instr *mi, FILE *o_fd)
{
    immed_list *rl = (immed_list *)mi->peek_annote(k_reloc);
    if (rl) {
        int i;
	for (i = 0; i < (rl->count() - 1); i++) {
	    fprintf(o_fd, "!");
	    Print_raw_immed(o_fd, (*rl)[i]);
	}
    }
    /* k_reloc left in annote list */
}


/*
 *  Methods in public "alpha_instr" classes.
 */

/* alpha_lab_print() -- print out label in Alpha format. */
void
alpha_lab_print(mi_lab *l, FILE *o_fd)
{
    l->label()->sym_node::print(o_fd);
    fprintf(o_fd,":");			/* Alpha-specific ending */
    alpha_print_reloc(l, o_fd);
    l->print_comment(o_fd, "#");
}


/* alpha_bj_print() -- print out instruction of mif_bj format.  Print
 * "rd, rs1, rs2, target" in that order, or "rd, rs" if indirect jump. */
void
alpha_bj_print(mi_bj *bj, FILE *o_fd)
{
    /* print out opcode and any extensions */
    alpha_print_opcode(bj, o_fd);

    /* print out destination if it exists */
    if (!bj->dst_op().is_null()) {
	assert_msg(bj->dst_op().is_reg(), ("alpha_bj_print() -- ",
	   "found bj dst_op that is neither NULL nor REG"));
        operand bj_d0 = bj->dst_op();
	alpha_print_operand(&bj_d0, o_fd);
	fprintf(o_fd,",");		/* prepare for target (at least) */
    }

    if (((int)bj->opcode()) == ao_call_pal) {
	/* only specifier is an immediate */
	operand bj_s0 = bj->src_op(0);
	alpha_print_operand(&bj_s0, o_fd);

    } else if (bj->is_indirect()) {
	/* operands for indirect target surrounded by ()'s */
	fprintf(o_fd,"(");
        operand bj_s0 = bj->src_op(0);
	alpha_print_operand(&bj_s0, o_fd);
	fprintf(o_fd,")");
	/* then print out the rest of the source registers, if any */
	for (unsigned i = 1; i < bj->num_srcs(); i++) {
	    if (!bj->src_op(i).is_null()) {
		operand bj_si = bj->src_op(i);
		alpha_print_operand(&bj_si, o_fd);
		fprintf(o_fd, ",");	/* prepare for target (at least) */
	    }
	}

    } else {
	/* first print out source registers, if any */
	for (unsigned i = 0; i < bj->num_srcs(); i++) {
	    if (!bj->src_op(i).is_null()) {
		operand bj_si = bj->src_op(i);
		alpha_print_operand(&bj_si, o_fd);
		fprintf(o_fd, ",");	/* prepare for target (at least) */
	    }
	}

	/* print out target address */
	Print_symbol(o_fd, bj->target());
    }

    if (((int)bj->opcode()) == ao_ret) fprintf(o_fd, ",1");	/* print hint */

    /* print more general hint */
    immed_list *il = (immed_list *)bj->peek_annote(k_hint);
    if (il) {
	assert(il->count() == 1);
	fprintf(o_fd, ",");
	Print_raw_immed(o_fd, (*il)[0]);
    }

    alpha_print_reloc(bj, o_fd);
    bj->print_comment(o_fd, "#");
}


/* alpha_rr_print() -- print out instruction of mif_rr format.  ALPHA
 * instructions that print out their operands in a weird order are
 * handled here. */
void
alpha_rr_print(mi_rr *r, FILE *o_fd)
{
    /* print out opcode and any extensions */
    alpha_print_opcode(r, o_fd);

    /* print out any operands -- some opcodes require special printing 
     * format for the operands. had to check for lda and ldgp since they
     * are printed like a load but do not read memory. */
    int r_opcode = r->opcode();
    if (Reads_memory(r) || (r_opcode == ao_lda) || (r_opcode == ao_ldah)
    || (r_opcode == ao_ldgp)) { 	/* is load */
	if ((r_opcode == ao_fetch) || (r_opcode == ao_fetch_m)) {
	    /* no destination in prefetch operation */
	} else {
	    assert(!r->dst_op().is_null());
            operand r_d0 = r->dst_op();
	    alpha_print_operand(&r_d0, o_fd);
	}
	fprintf(o_fd, ",");

	/* print out effective address operand */
	assert(r->num_srcs() > 0);
        operand r_s0 = r->src_op(0);
	alpha_print_operand(&r_s0, o_fd);

    } else if (Writes_memory(r)) {	/* is store */
	assert(r->dst_op().is_null());
	assert((r->num_srcs() > 1) && !r->src_op(1).is_null());
        operand r_s1 = r->src_op(1);
	alpha_print_operand(&r_s1, o_fd);
	fprintf(o_fd, ",");

	/* print out effective address operand--in srcs[0] by convention */
        operand r_s0 = r->src_op(0);
	alpha_print_operand(&r_s0, o_fd);

    } else if (alpha_is_ldc(r)) {
	/* print out "dest, imm" in that order */
	operand r_d0 = r->dst_op();
	alpha_print_operand(&r_d0, o_fd);
	fprintf(o_fd, ",");
	assert((r->num_srcs() > 0) && !r->src_op(0).is_null());
        operand r_s0 = r->src_op(0);
	alpha_print_operand(&r_s0, o_fd);

    } else if (alpha_is_cmove(r)) {
	// don't print out the last source which corresponds to
	// the destination -- needed for correct liveness analysis
	operand r_s0 = r->src_op(0);
	alpha_print_operand(&r_s0, o_fd);
	fprintf(o_fd, ",");
        operand r_s1 = r->src_op(1);
	alpha_print_operand(&r_s1, o_fd);
	fprintf(o_fd, ",");
        operand r_d0 = r->dst_op();
	alpha_print_operand(&r_d0, o_fd);

    } else {
	/* print out "src1, src2, dest" in that order */
	boolean need_comma = FALSE;
	for (unsigned i = 0; i < r->num_srcs(); i++) {
	    operand s = r->src_op(i);
	    if (!s.is_null()) {
		if (need_comma) fprintf(o_fd, ",");
		else need_comma = TRUE;		/* next time */
		alpha_print_operand(&s, o_fd);
	    }
	}
	if (!r->dst_op().is_null()) {
	    if (need_comma) fprintf(o_fd, ",");
            operand r_d0 = r->dst_op();
	    alpha_print_operand(&r_d0, o_fd);
	}
    }

    alpha_print_reloc(r, o_fd);
    r->print_comment(o_fd, "#");
}


/* alpha_xx_print() -- print out instruction of mif_xx format */
void
alpha_xx_print(mi_xx *x, immed_list *il, FILE *o_fd)
{
    immed_list_iter ili(il);

    /* Special case ao_null so we don't output null lines */
    if (((int)x->opcode()) == ao_null) {
	if (x->are_annotations()) x->print_comment(o_fd, "#");
	return;
    }

    assert_msg((x->get_annote(k_instr_op_exts) == NULL),
	       ("alpha_xx_print() -- unexpected k_instr_op_exts"));

    /* print out opcode */
    fprintf(o_fd, "\t%s\t", x->op_string());

    switch (x->opcode()) {
      case ao_file:			/* .file <file_no> "<file_name>" */
	assert(il->count() == 2);
	fprintf(o_fd,"%d \"", ili.step().integer());	/* file no. */
	fprintf(o_fd,"%s\"", ili.step().string());	/* file name */
	break;

      case ao_frame:			/* .frame $sp,framesize,$26 */
	assert((il->count() == 3) || (il->count() == 4));
	assert(ili.step().integer() == 30);
	fprintf(o_fd,"$sp, %d, $26", ili.step().integer());
	assert(ili.step().integer() == 26);
	if (il->count() == 4) fprintf(o_fd,", %d", ili.step().integer());
	break;

      case ao_mask: case ao_fmask:	/* .*mask <hex_mask>, <f_offset> */
	assert(il->count() == 2);
	fprintf(o_fd,"0x%08x, ", ili.step().integer()); /* hex mask */
	fprintf(o_fd,"%d", ili.step().integer());	/* frame offset */
	break;

      case ao_ascii: case ao_asciiz:	/* .ascii* ["<s1>"[, "<s2>"...]] */
	while (!ili.is_empty()) {
	    /* while >1 element in list, print with comma */
	    fprintf(o_fd,"\"%s\"", ili.step().string());
	    if (!ili.is_empty()) fprintf(o_fd,", ");
	}
	break;

      /* Print arguments without commas and strings without quotes */
      case ao_ent: case ao_extern: case ao_livereg:
      case ao_loc: case ao_verstamp: case ao_vreg:
      	while (!ili.is_empty()) {
	    Print_raw_immed(o_fd, ili.step());
	    fprintf(o_fd, " ");
	}
	break;

      /* Print arguments with commas and strings without quotes */
      default:
	while (!ili.is_empty()) {
	    /* while >1 element in list, print with comma */
	    Print_raw_immed(o_fd, ili.step());
	    if (!ili.is_empty()) fprintf(o_fd,", ");
	}
	break;
    }

    alpha_print_reloc(x, o_fd);
    x->print_comment(o_fd, "#");
}


/*
 *  Other helpful print methods that are architecture-specific.
 */

/* alpha_print_global_directives() -- if we inserted relocations, we
 * must disable code reordering by the assembler.  If we do not, it
 * will reorder the instructions including those that define $gp.  Since
 * these instructions depend upon their placement, do not move them! */
void
alpha_print_global_directives(file_set_entry *fse, FILE *o_fd)
{
    immed_list *reloc_iml;
    reloc_iml = (immed_list *)fse->peek_annote(k_next_free_reloc_num);
    if (reloc_iml) {
	// This annotation exists only if we inserted relocations.  So,
	// disallow code reordering and also globally allow for the
	// use of the $at register.
	mi_xx *mi = new mi_xx(ao_set, immed("noreorder"));
	mi->print(o_fd);
	delete mi;
	mi = new mi_xx(ao_set, immed("noat"));
	mi->print(o_fd);
	delete mi;
    }
    // else, nothing to do
}


/* alpha_print_extern_op() -- */
void
alpha_print_extern_op(var_sym *v, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(ao_extern, immed(v));
    mi->append_operand(immed(v->type()->size() >>
			     SHIFT_BITS_TO_BYTES));
    mi->print(o_fd);
    delete mi;
}


/* alpha_print_file_op() -- */
void
alpha_print_file_op(int fnum, char *fnam, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(ao_file, immed(fnum));
    mi->append_operand(immed(fnam));
    mi->print(o_fd);
    delete mi;
}


/* alpha_print_var_def() -- Generates assembly language data statements.
 * Data objects are put into the sdata section, data section, comm section,
 * or the lcomm section.  If the data object is global, I also specify
 * a .globl pseudo-op.
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
alpha_print_var_def(var_sym *vsym, int Gnum, FILE *o_fd)
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
	if ((an = anl->peek_annote(k_repeat_init))) {
	    /* check if repeat value is 0 or 0.0 */
	    immed i = (*(an->immeds()))[2];	/* repeat value */
	    if (i.is_integer()) {
		if (i.integer()) anl->append(an);	/* test failed */
	    } else if (i.is_flt()) {
		if (i.flt() != 0.0) anl->append(an);	/* test failed */
	    } else {
		anl->push(an);				/* test failed */
	    }

	} else if ((an = anl->peek_annote(k_fill))) {
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
	/* Specify data section to place data in. By default, if the
	 * data is greater than 512 bytes in size, we use .data,
	 * otherwise we use .sdata.  The default size can be changed
	 * by specifing a new number with the -G option. */
	if ((vsym->type()->size() >> SHIFT_BITS_TO_BYTES) > Gnum)
	    mi = new mi_xx(ao_data);
	else
	    mi = new mi_xx(ao_sdata);
	mi->print(o_fd); delete mi;

	/* If global symbol, add .globl pseudo-op.  Note that a variable
	 * is considered global in SUIF if it is part of the global or
	 * file symbol table.  For Alpha assembly, a datum is global only
	 * if it is part of the global symbol table (i.e. is_private()
	 * checks for def in file symbol table). */
	if (vsym->is_global() && !vsym->is_private()) {
	    mi = new mi_xx(ao_globl,immed(vsym));
	    mi->print(o_fd); delete mi;
	}

	/* Determine alignment and align */
	mi = new mi_xx(ao_align);
	switch (vdef->alignment()) {
	  case SIZE_OF_BYTE:			/* byte aligned */
	    mi->append_operand(immed(0));
	    break;
	  case SIZE_OF_HALFWORD:		/* halfword aligned */
	    mi->append_operand(immed(1));
	    break;
	  case SIZE_OF_WORD:			/* word aligned */
	    /* alpha defaults int alignment to double-word alignment */
	    mi->append_operand(immed(3));
	    break;
	  case SIZE_OF_DOUBLE:			/* double word aligned */
	    mi->append_operand(immed(3));
	    break;
	  case SIZE_OF_QUAD:			/* quad word aligned */
	    mi->append_operand(immed(4));
	    break;
	  default:
	    assert_msg(FALSE,
		("alpha_print_var_def() -- bad alignment for %s",
		 vsym->name()));
	}
	mi->print(o_fd); delete mi;

	/* Disable automatic alignment */
	mi = new mi_xx(ao_align,immed(0));
	il = new immed_list();
	il->append(immed("disable automatic alignment"));
	mi->append_comment(il);
	mi->print(o_fd); delete mi;

	/* Write out the label */
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

		    /* NOTE: Even though the assembler manual makes it
		     * look like you can create an .ascii pseudo-op with
		     * multiple operands, you can't.  The assembler
		     * barfs.  Oh, goody. */

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
			    data_string[ds_i] = '\0';	/* end current str */
			    mi = new mi_xx(ao_ascii, immed(data_string));
			    mi->print(o_fd);
			    delete mi;
			    ds_i = 0;			/* reset for new str */
			}

			/* insert value into current string as char */
			if (c == (int)'\\') {
			    data_string[ds_i++] = '\\';
			    data_string[ds_i++] = '\\';
			} else if ((c >= (int)' ') && (c != (int)'"')
				   && (c <= (int)'~')) {
			    data_string[ds_i++] = (char)c;
			} else {
			    char hex_value[3];
			    data_string[ds_i++] = '\\';
			    data_string[ds_i++] = 'X';
			    sprintf(hex_value,"%02x",(unsigned char)c);
			    data_string[ds_i++] = hex_value[0];
			    data_string[ds_i++] = hex_value[1];
			}
		    }

		    /* end current string */
		    data_string[ds_i] = '\0';
		    mi = new mi_xx(ao_ascii, immed(data_string));
		    mi->print(o_fd);
		    delete mi;

		} else 
		    assert_msg(FALSE,
			("alpha_print_var_def() -- "
			 "k_multi-init value size = %d", value_size));

	    } else if (an->name() == k_repeat_init) {
		int repeat_cnt = (*il)[0].integer();
		int value_size = (*il)[1].integer();
		immed ivalue = (*il)[2];

		if (repeat_cnt > 1) {	/* insert .repeat pseudo-op */
		    mi = new mi_xx(ao_repeat,immed(repeat_cnt));
		    mi->print(o_fd); delete mi;
		}

		/* actual data op */
		switch (value_size) {		/* to get value size */
		  case SIZE_OF_BYTE:
		    mi = new mi_xx(ao_byte, ivalue);
		    break;
		  case SIZE_OF_HALFWORD:
		    mi = new mi_xx(ao_word, ivalue);
		    break;
		  case SIZE_OF_WORD:
		    if (ivalue.is_integer() || ivalue.is_ext_integer())
			mi = new mi_xx(ao_long, ivalue);
		    else if (ivalue.is_flt() || ivalue.is_ext_flt())
			/* got a FP single */
			mi = new mi_xx(ao_s_floating, ivalue);
		    else if (ivalue.is_symbol())
			mi = new mi_xx(ao_long, ivalue);
		    else
			assert_msg(FALSE,
			    ("alpha_print_var_def() -- "
			     "bad k_repeat_init value"));
		    break;
		  case SIZE_OF_DOUBLE:
		    if (ivalue.is_flt() || ivalue.is_ext_flt())
			mi = new mi_xx(ao_t_floating, ivalue);
		    else
			mi = new mi_xx(ao_quad, ivalue);
		    break;
		  default:
		    assert_msg(FALSE,
		        ("alpha_print_var_def() -- "
			 "bad size for k_repeat_init"));
		}
		mi->print(o_fd); delete mi;

		if (repeat_cnt > 1) {	/* insert .endr pseudo-op */
		    mi = new mi_xx(ao_endr);
		    mi->print(o_fd); delete mi;
		}

	    } else if (an->name() == k_fill) {
		/* fill size with bit pattern, assuming size is a multiple
		 * of bytes and the bit pattern is all zeros. */
		int value_size = (*il)[0].integer();
		assert((value_size & 7) == 0);
		assert((*il)[1].integer() == 0);

		/* insert .repeat pseudo-op */
		mi = new mi_xx(ao_repeat,
			       immed(value_size >> SHIFT_BITS_TO_BYTES));
		mi->print(o_fd); delete mi;

		/* insert .byte 0 */
		mi = new mi_xx(ao_byte,(*il)[1]);
		mi->print(o_fd); delete mi;

		/* insert .endr pseudo-op */
		mi = new mi_xx(ao_endr);
		mi->print(o_fd); delete mi;

	    } else if (an->name() == k_gprel_init) {
		/* Alpha-specific initialization -- generate a .gprel32
		 * pseudo-op for the var_sym.  Format of annotation is
		 * value_size followed by symbol whose offset we want. */
		int value_size = (*il)[0].integer();
		assert(value_size == 32);	/* only recognized size */
		assert(il->count() == 2);
		immed avalue = (*il)[1];
		mi = new mi_xx(ao_gprel32, avalue);
		mi->print(o_fd); delete mi;

	    } else
	        warning_line(NULL, "alpha_print_var_def() -- %s (%s)"
		        "ignoring unknown var_def annote", an->name());
	}

    } else {				/* uninitialized data item */
	/* if global symbol, append to .comm, else static symbol to .lcomm */
	if (vsym->is_global() && !vsym->is_private())
	    mi = new mi_xx(ao_comm, immed(vsym));
	else
	    mi = new mi_xx(ao_lcomm, immed(vsym));
	mi->append_operand(immed(vsym->type()->size() >> SHIFT_BITS_TO_BYTES));
	mi->print(o_fd); delete mi;
   }
}


/* alpha_print_proc_def() -- */
void
alpha_print_proc_def(proc_sym *p, FILE *o_fd)
{
    /* do nothing */
}


/* alpha_print_proc_begin() -- Starts a procedure text segment. */
void
alpha_print_proc_begin(proc_sym *psym, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(ao_text);				/* .text */
    mi->print(o_fd);
    delete mi;
    mi = new mi_xx(ao_align,immed(4));				/* .align 4 */
    mi->print(o_fd);
    delete mi;
}


/* alpha_print_proc_entry() -- Reads the symbol table information on
 * the proc_sym and the annotations off the tree_proc.  It creates the
 * correct Alpha instructions to start a procedure text segment. */
void
alpha_print_proc_entry(proc_sym *psym, int file_no_for_1st_line, FILE *o_fd)
{
    mi_xx *mi;
    mi = new mi_xx(ao_align,immed(4));				/* .align 4 */
    mi->print(o_fd);
    delete mi;

    /* if global procedure, then add this pseudo-op.  I.e. procedure is
     * in global symbol table, not in the file symbol table. */
    if (psym->is_global() && !psym->is_private()) {
	mi = new mi_xx(ao_globl,immed(psym->name()));		/* .globl */
	mi->print(o_fd);
	delete mi;
    }

    /* get file and line no. info for this procedure -- .file annote
     * already generated (if necessary) in Process_file() */
    immed_list *iml = (immed_list *)psym->block()->peek_annote(k_line);
    assert(iml && (strcmp((*iml)[1].string(),"") != 0));
    int first_line_no = (*iml)[0].integer();
    mi = new mi_xx(ao_loc,immed(file_no_for_1st_line));		/* .loc */
    mi->append_operand(immed(first_line_no));
    mi->print(o_fd);
    delete mi;

    mi = new mi_xx(ao_ent,immed(psym->name()));			/* .ent */
    mi->print(o_fd);
    delete mi;

    /* the procedure symbol cannot be a label symbol since SUIF does not
     * allow label symbols outside of a procedural context (this is the
     * reason why this routine exists in the first place), and so, we
     * have to generate the procedure label in a funny way. */
    fprintf(o_fd, "%s:\n", psym->name());			/* proc: */

    /* .frame (and others if not leaf) added to text during code gen */
}


/* alpha_print_proc_end() -- outputs the .end pseudo-op. */
void
alpha_print_proc_end(proc_sym *psym, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(ao_end, immed(psym->name()));		/* .end */
    mi->print(o_fd);
    delete mi;
}
