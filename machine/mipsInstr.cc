/* file "mipsInstr.cc" */

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


static void mips_print_opcode(machine_instr *, FILE *);
/*static*/ void mips_print_operand(operand *, FILE *);
static void mips_print_ea_s0_p_s1(instruction *, FILE *);
static void mips_print_ea_operand(operand *, FILE *);
static void mips_print_reloc(machine_instr *, FILE *);


/*
 *  The following are helper definitions and classes that are
 *  basically a cheap way of implementing the (unimplemented)
 *  base class mips_instr.
 */

char *k_mips;
//char *k_gprel_init;


/* init_mips() -- do all of the priliminary stuff required to
 * use the mips instruction classes. */
void
init_mips()
{
    /* setup useful string constants */
    k_mips = lexicon->enter("mips")->sp;

    //ANNOTE(k_gprel_init, "gprel_init", TRUE);
}


/* ---------------- Is_* helper routines ------------------- */

boolean
mips_is_ldc(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == mo_li) || (o == mo_lui)||(o==mo_li_d)||(o==mo_li_s)); 
}

boolean
mips_is_move(instruction *i)
{
    int o = (int)i->opcode();
    if ((o == mo_move) || (o == mo_mfhi)||(o==mo_mfhi)||(o==mo_mflo)
	||(o==mo_mthi)||(o==mo_mtlo)||(o==mo_mfc0)||(o==mo_mfc1)
	||(o==mo_mfc1_d)||(o==mo_mtc0)||(o==mo_mtc1)||(o==mo_mov_d)
	||(o==mo_mov_s))
	return TRUE;
    else
	return FALSE;
}

boolean
mips_is_cmove(instruction *i)
{
    int o = (int)i->opcode();
    if ((o==mo_movn)||(o==mo_movz)||(o==mo_movf)
	||(o==mo_movt) ||(o==mo_movf_d)||(o==mo_movf_s)
	||(o==mo_movt_d)||(o==mo_movt_s)||(o==mo_movn_d)
	||(o==mo_movn_s)||(o==mo_movz_d)||(o==mo_movz_s))
	return TRUE;
    else
	return FALSE;
}

boolean
mips_is_line(instruction *i)
{
    int o = (int)i->opcode();
    if (o == mo_loc)
	return TRUE;
    else
	return FALSE;
}

boolean
mips_is_ubr(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == mo_b) || (o == mo_j)/*||(o==mo_jal)||(o==mo_jalr)
		   ||(o==mo_jr)*/)
	&& (!i->peek_annote(k_instr_ret))
	&& (!i->peek_annote(k_instr_mbr_tgts)); 
}

boolean
mips_is_cbr(instruction *i)
{
    int o = (int)i->opcode();
    return ((o==mo_bc1f)||(o==mo_bc1t)||(o==mo_beq)||(o==mo_bgez)
		   ||(o==mo_bgezal)||(o==mo_bgtz)||(o==mo_blez)||(o==mo_bltzal)
		   ||(o==mo_bltz)||(o==mo_bne)||(o==mo_beqz)||(o==mo_bge)
		   ||(o==mo_bgeu)||(o==mo_bgt)||(o==mo_bgtu)||(o==mo_ble)
		   ||(o==mo_bleu)||(o==mo_blt)||(o==mo_bltu)||(o==mo_bnez)); 
}

boolean
mips_is_call(instruction *i)
{
    int o = (int)i->opcode();
    return ((o == mo_jal) || (o == mo_jalr) ); 
}

boolean
mips_is_return(instruction *i)
{
    int o = (int)i->opcode();
    return (o == mo_jr); 
}

boolean
mips_reads_memory(instruction *i)
{
    int o = (int)i->opcode();
    return ((o==mo_lb)||(o==mo_lbu)||(o==mo_lh)||(o==mo_lhu)
	          || (o==mo_lw)||(o==mo_lwc1)||(o==mo_lwl)||(o==mo_lwr)
	          ||(o==mo_ld)||(o==mo_ulh)||(o==mo_ulhu)||(o==mo_ulw)
	          ||(o==mo_ll)); 
}


/* -------------- Code generation helper routines ----------------- */

mi_ops
mips_load_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
	assert(t->op() != TYPE_FLOAT);
	if (t->is_base())
	    return ((base_type *)t)->is_signed() ? mo_lb: mo_lbu;
	else
	    return mo_lbu;

      case SIZE_OF_HALFWORD:
	assert(t->op() != TYPE_FLOAT);
	if (t->is_base())
	    return ((base_type *)t)->is_signed() ? mo_lh : mo_lhu;
	else
	    return mo_lhu;

      case SIZE_OF_WORD:
	return (t->op() == TYPE_FLOAT) ? mo_l_s : mo_lw;

      case SIZE_OF_DOUBLE:
	return (t->op() == TYPE_FLOAT) ?  mo_l_d: mo_ld;

      default:
	t->print_full(stderr);
	assert_msg(FALSE, ("Load_op() -- Unknown load size for io_lod."));
	return mo_null;
    }
}

mi_ops
mips_store_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
	assert(t->op() != TYPE_FLOAT);
	return mo_sb;

      case SIZE_OF_HALFWORD:
	assert(t->op() != TYPE_FLOAT);
	return mo_sh;

      case SIZE_OF_WORD:
	return (t->op() == TYPE_FLOAT) ? mo_s_s : mo_sw;

      case SIZE_OF_DOUBLE:
	return (t->op() == TYPE_FLOAT) ? mo_s_d : mo_sd;

      default:
	t->print_full(stderr);
	assert_msg(FALSE, ("Store_op() -- Unknown load size for io_str."));
	return mo_null;
    }
}

mi_ops
mips_move_op(type_node *t)
{
    switch (t->size()) {
      case SIZE_OF_BYTE:
      case SIZE_OF_HALFWORD:
	t->print_full(stderr);
	assert_msg(FALSE, ("Move_op() -- Unexpected size for move."));
	return mo_null;                  

      case SIZE_OF_WORD:
	  return (t->op() == TYPE_FLOAT) ? mo_mov_s : mo_move;
      case SIZE_OF_DOUBLE:
	assert_msg((t->op() == TYPE_FLOAT),
		   ("Move_op() -- can only move 64-bit FP numbers"));
	return mo_mov_d;


      default:
	t->print_full(stderr);
	assert_msg(FALSE, ("Move_op() -- Unexpected size for move."));
	return mo_null;
    }
	 
}

/* ---------------- print helper routines ------------------- */

/* mips_print_opcode() -- prints out the machine opcode and any extensions
 * in mips-specific syntax. */
static void
mips_print_opcode(machine_instr *i, FILE *o_fd)
{
    immed_list *iml;
    fprintf(o_fd, "\t%s", i->op_string());
    if ((iml = (immed_list *)i->peek_annote(k_instr_op_exts))) {
	immed_list_iter ili(iml);
	while (!ili.is_empty()) {
	    immed im =  ili.step();
	    fprintf(o_fd, "%s", mips_op_ext_string(im.integer()));
	    /* normally, we'd print the extension separator each time
	     * too, but mips has none */
	}
    }
    fprintf(o_fd, "\t");
}


/* mips_print_ea_s0_p_s1() -- helper routine for mips_print_ea_operand(). */
static void
mips_print_ea_s0_p_s1(instruction *in, FILE *o_fd)
{
    assert(in->opcode() == io_add);
    operand in_s0 = in->src_op(0);
    operand in_s1 = in->src_op(1);

    /* print first operand */
    mips_print_operand(&in_s0, o_fd);

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
	    assert("mips_print_ea_s0_p_s1() -- "
		   "unexpected immed type in instruction");
	}
    } else
	fprintf(o_fd, "+");

    /* print second operand, if non-zero */
    if (!opd2_is_0)
	mips_print_operand(&in_s1, o_fd);
}

/* mips_print_ea_operand() -- prints out an effective address operand
 * in mips-specific syntax. This routine expects the EA operand as
 * an expression, but it can handle flat lists of machine instructions,
 * i.e. lists with SUIF and machine instructions. */
static void
mips_print_ea_operand(operand *o, FILE *o_fd)
{
    assert(o->is_expr() || o->is_instr());
    instruction *ea = o->instr();
    if_ops op = ea->opcode();

    if (Is_ea_base_p_disp(ea)) {
	operand ea_s0 = ea->src_op(0);
	if (ea_s0.is_reg()) {
	    /* print as 'offset(base_reg)' -- offset already in bytes */
            operand ea_s1 = ea->src_op(1);
	    mips_print_operand(&ea_s1, o_fd);
	    fprintf(o_fd,"(");
	    mips_print_operand(&ea_s0, o_fd);
	    fprintf(o_fd,")");

	} else {
	    /* print as 's0+s1' */
	    mips_print_ea_s0_p_s1(ea, o_fd);
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
	mips_print_ea_s0_p_s1(ea_s1.instr(), o_fd);

	/* print index register */
	fprintf(o_fd,"(");
	mips_print_operand(&ea_s0, o_fd);
	fprintf(o_fd,")");

    } else {
	assert_msg(FALSE, ("mips_print_ea_operand() -- "
			   "unexpected ea kind (opcode = %s)",
			   if_ops_name(op)));
    }
}


/* mips_print_operand() -- prints out an operand in mips-specific
 * syntax. */
/*static*/ void
mips_print_operand(operand *o, FILE *o_fd)
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
	if (Is_ea_operand(*o)) mips_print_ea_operand(o, o_fd);
	else Print_raw_immed(o_fd, o->immediate());
	break;

      default:
	assert_msg(FALSE,
	    ("mips_print_operand() -- unknown kind %d", o->kind()));
    }
}


/* mips_print_reloc() -- prints out the relocation information in
 * mips-specific syntax.  The last value in the immediate list is
 * the index of the source operand to which this relocation refers.
 * It is not printed. */
static void 
mips_print_reloc(machine_instr *mi, FILE *o_fd)
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
 *  Methods in public "mips_instr" classes.
 */

/* mips_lab_print() -- print out label in mips format. */
void
mips_lab_print(mi_lab *l, FILE *o_fd)
{
    l->label()->sym_node::print(o_fd);
    fprintf(o_fd,":");			/* mips-specific ending */
    mips_print_reloc(l, o_fd);
    l->print_comment(o_fd, "#");
}


/* mips_bj_print() -- print out instruction of mif_bj format.  Print
 * " rs1, rs2, target" in that order, or " rd,rs" if indirect jump. */
void
mips_bj_print(mi_bj *bj, FILE *o_fd)
{
   //debug(99,"Printing mips instruction:%s in mips_bj_print()",bj->op_string());
   //printf("\nPrinting mips instruction:%s in mips_bj_print()\n",bj->op_string());
    /* print out opcode and any extensions */
    mips_print_opcode(bj, o_fd);

  if (bj->is_indirect()) {
  	assert_msg(bj->opcode()==(if_ops)mo_jalr||bj->opcode()==(if_ops)mo_jr, ("mips_bj_print() -- ",
	   "only jalr and bj can have indirect jump in bj instruction"));
	/* operands for indirect target  */
	operand bj_s0 = bj->src_op(0);
	mips_print_operand(&bj_s0, o_fd);
	/*  prepare for printing out the destination register, if it is jalr instruction */
      /* print out destination if it exists */
    if(bj->opcode()==(if_ops)mo_jalr)
	        fprintf(o_fd, ",");
    if (!bj->dst_op().is_null()) {
	assert_msg(bj->opcode()==(if_ops)mo_jalr, ("mips_bj_print() -- ",
	   "only jalr can have destination register in bj instruction"));
	assert_msg(bj->dst_op().is_reg(), ("mips_bj_print() -- ",
	   "found bj dst_op that is neither NULL nor REG"));
	  
        operand bj_d0 = bj->dst_op();
	mips_print_operand(&bj_d0, o_fd);
       }
	
	
			
    } else {
	/* first print out source registers, if any */
	for (unsigned i = 0; i < bj->num_srcs(); i++) {
	    if (!bj->src_op(i).is_null()) {
		operand bj_si = bj->src_op(i);
		mips_print_operand(&bj_si, o_fd);
		fprintf(o_fd, ",");	/* prepare for target (at least) */
	    }
	}
	 
  	/* print out target symbol address */
    //if ((bj->opcode()!=(if_ops)mo_j)&&(bj->opcode()!=(if_ops)mo_jal))/*We treat the target of j and jal instructions as source operands*/	
	Print_symbol(o_fd, bj->target());
    }

    mips_print_reloc(bj, o_fd);
    bj->print_comment(o_fd, "#");
}


/* mips_rr_print() -- print out instruction of mif_rr format.  mips
 * instructions that print out their operands in a weird order are
 * handled here. */
void
mips_rr_print(mi_rr *r, FILE *o_fd)
{
    mips_print_opcode(r, o_fd);

    /* print out any operands -- some opcodes require special printing 
     * format for the operands */
    int r_opcode = r->opcode();

   /*Notice:for divide command,we currently use pseudo-ops "div rdest,rsrc1,src2" and 
   "divu rdest,rsrc1,src2" instead of use machine instruction "div rs,rt" and "divu rs,rt"(we transfer this
   responsiblilty to the assembler)!*/	
    if (r_opcode==mo_mult||r_opcode==mo_multu||r_opcode==mo_madd||r_opcode==mo_maddu
	  ||r_opcode==mo_msub||r_opcode==mo_msubu) 
       { 
         /* print out "src1, src2" in that order -- no destination */
	 // assert_msg(r->num_srcs() ==2&&r->num_dsts()==0, ("mips_rr_print() -- ",
	//   "this opcode  can have only 2 source operand and no destination operand in  instruction"));
	    operand r_s0 =r->src_op(0);
	    mips_print_operand(&r_s0, o_fd);
	    fprintf(o_fd, ",");
	    operand r_s1 =r->src_op(1);	
	    mips_print_operand(&r_s1, o_fd);	
	  	
       } 
   else if(r_opcode==mo_mthi||r_opcode==mo_mtlo)
      {
        /* print out "src1" in that order -- no destination */
	assert_msg(r->num_srcs() ==1&&r->num_dsts()==0, ("mips_rr_print() -- ",
	   "this opcode  can have only 1 source operand and no destination operand in  instruction"));
	  operand r_s0 =r->src_op(0);
	   mips_print_operand(&r_s0, o_fd);	
      }
    else if (r_opcode==mo_mtc0||r_opcode==mo_mtc1) 
     {
     //assert_msg(r->num_srcs() ==1&&r->num_dsts()==1, ("mips_rr_print() -- ",
//	   "this opcode  can have only 1 source operand and 1 destination operand in  instruction"));
	/* print specifiers in reverse order, i.e. src then dst */
	operand r_s0 =r->src_op(0);
	mips_print_operand(&r_s0, o_fd);
	fprintf(o_fd, ",");
	operand r_d0 = r->dst_op();
	mips_print_operand(&r_d0, o_fd);

    } else if (r_opcode==mo_mfhi||r_opcode==mo_mflo) 
    {
	/* print destination */ 
	// assert_msg(r->num_srcs() ==0&&r->num_dsts()==1, ("mips_rr_print() -- ",
	//   "this opcode  can have only no source operand and 1 destination operand in  instruction"));
 	  assert(!r->dst_op().is_null());
            operand r_d0 = r->dst_op();
	    mips_print_operand(&r_d0, o_fd);

    } else if (Reads_memory(r) || (r_opcode == mo_la) ) { 	/* is load */
	    assert(!r->dst_op().is_null());
            operand r_d0 = r->dst_op();
	    mips_print_operand(&r_d0, o_fd);
	    fprintf(o_fd, ",");

	  /* print out effective address operand */
	   assert(r->num_srcs() > 0);
          operand r_s0 = r->src_op(0);
	   mips_print_operand(&r_s0, o_fd);

        } else if (Writes_memory(r)) {	/* is store */
	assert(r->dst_op().is_null());
	assert((r->num_srcs() > 1) && !r->src_op(1).is_null());
        operand r_s1 = r->src_op(1);
	mips_print_operand(&r_s1, o_fd);
	fprintf(o_fd, ",");

	/* print out effective address operand--in srcs[0] by convention */
        operand r_s0 = r->src_op(0);
	mips_print_operand(&r_s0, o_fd);

    } else if (mips_is_ldc(r)) {
	// print out "dest, imm" in that order 
	operand r_d0 = r->dst_op();
	mips_print_operand(&r_d0, o_fd);
	fprintf(o_fd, ",");
	assert((r->num_srcs() > 0) && !r->src_op(0).is_null());
        operand r_s0 = r->src_op(0);
	mips_print_operand(&r_s0, o_fd);

    } /*else if (mips_is_cmove(r)) {
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

    } */else {
	/* print out "dest, src1, src2" or "dest, src1" in that order */
	    if (!r->dst_op().is_null()) {
	     operand r_d0 = r->dst_op();
	     mips_print_operand(&r_d0, o_fd);
	     fprintf(o_fd, ",");
	    }
	    boolean need_comma = FALSE;
	    for (unsigned i = 0; i < r->num_srcs(); i++) {
	          operand s = r->src_op(i);
	          if (!s.is_null()) {
		       if (need_comma) fprintf(o_fd, ",");
		       else need_comma = TRUE;		/* next time */
		       mips_print_operand(&s, o_fd);
	          }
	   }
    }

	
    mips_print_reloc(r, o_fd);
    r->print_comment(o_fd, "#");
 }


/* mips_xx_print() -- print out instruction of mif_xx format */
void
mips_xx_print(mi_xx *x, immed_list *il, FILE *o_fd)
{
    immed_list_iter ili(il);

    /* Special case ao_null so we don't output null lines */
    if (((int)x->opcode()) == mo_null) {
	if (x->are_annotations()) x->print_comment(o_fd, "#");
	return;
    }

    assert_msg((x->get_annote(k_instr_op_exts) == NULL),
	       ("mips_xx_print() -- unexpected k_instr_op_exts"));

    /* print out opcode */
    fprintf(o_fd, "\t%s\t", x->op_string());

    switch (x->opcode()) {
      case mo_file:			/* .file <file_no> "<file_name>" */
	assert(il->count() == 2);
	fprintf(o_fd,"%d \"", ili.step().integer());	/* file no. */
	fprintf(o_fd,"%s\"", ili.step().string());	/* file name */
	break;

      case mo_frame:			/* .frame $sp,framesize,$26 */
	assert(il->count() == 3) ;
	assert(ili.step().integer() == 29);
	fprintf(o_fd,"$sp, %d, $31", ili.step().integer());
	assert(ili.step().integer() == 31);
	break;

      case mo_mask: case mo_fmask:	/* .*mask <hex_mask>, <f_offset> */
	assert(il->count() == 2);
	fprintf(o_fd,"0x%08x, ", ili.step().integer()); /* hex mask */
	fprintf(o_fd,"%d", ili.step().integer());	/* frame offset */
	break;

      case mo_ascii: case mo_asciiz:	/* .ascii* ["<s1>"[, "<s2>"...]] */
	while (!ili.is_empty()) {
	    /* while >1 element in list, print with comma */
	    fprintf(o_fd,"\"%s\"", ili.step().string());
	    if (!ili.is_empty()) fprintf(o_fd,", ");
	}
	break;

      /* Print arguments without commas and strings without quotes */
      case mo_ent: case mo_end: case mo_aent:
      case mo_text: case mo_rdata: case mo_data:
      case mo_sdata:
      	while (!ili.is_empty()) {
	    Print_raw_immed(o_fd, ili.step());
	    fprintf(o_fd, " ");
	}
	break;

      /* Print arguments with commas and strings without quotes */
      default:/*Process the following pseudo-ops:
      .byte,.half,.word,.dword,.float,.double,.align,.space,.global,.extern,.weakext,
      .set*/
	while (!ili.is_empty()) {
	    /* while >1 element in list, print with comma */
	    Print_raw_immed(o_fd, ili.step());
	    if (!ili.is_empty()) fprintf(o_fd,", ");
	}
	break;
    }

    mips_print_reloc(x, o_fd);
    x->print_comment(o_fd, "#");
}


/*
 *  Other helpful print methods that are architecture-specific.
 */

/* mips_print_global_directives() -- if we inserted relocations, we
 * must disable code reordering by the assembler.  If we do not, it
 * will reorder the instructions including those that define $gp.  Since
 * these instructions depend upon their placement, do not move them! */
void
mips_print_global_directives(file_set_entry *fse, FILE *o_fd)
{
    immed_list *reloc_iml;
	mi_xx *mi = new mi_xx(mo_set, immed("noreorder"));
	mi->print(o_fd);
	delete mi;
    reloc_iml = (immed_list *)fse->peek_annote(k_next_free_reloc_num);
    if (reloc_iml) {
	// This annotation exists only if we inserted relocations.  So,
	// disallow code reordering and also globally allow for the
	// use of the $at register.
	mi_xx *mi = new mi_xx(mo_set, immed("noreorder"));
	mi->print(o_fd);
	delete mi;
	mi = new mi_xx(mo_set, immed("noat"));
	mi->print(o_fd);
	delete mi;
    }
    // else, nothing to do
}


/* mips_print_extern_op() -- */
void
mips_print_extern_op(var_sym *v, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(mo_extern, immed(v));
    mi->append_operand(immed(v->type()->size() >>
			     SHIFT_BITS_TO_BYTES));
    mi->print(o_fd);
    delete mi;
}


/* mips_print_file_op() -- */
void
mips_print_file_op(int fnum, char *fnam, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(mo_file, immed(fnum));
    mi->append_operand(immed(fnam));
    mi->print(o_fd);
    delete mi;
}


/* mips_print_var_def() -- Generates assembly language data statements.
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
mips_print_var_def(var_sym *vsym, int Gnum, FILE *o_fd)
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
	    mi = new mi_xx(mo_data);
	else
	    mi = new mi_xx(mo_sdata);
	mi->print(o_fd); delete mi;

	/* If global symbol, add .globl pseudo-op.  Note that a variable
	 * is considered global in SUIF if it is part of the global or
	 * file symbol table.  For mips assembly, a datum is global only
	 * if it is part of the global symbol table (i.e. is_private()
	 * checks for def in file symbol table). */
	if (vsym->is_global() && !vsym->is_private()) {
	    mi = new mi_xx(mo_globl,immed(vsym));
	    mi->print(o_fd); delete mi;
	}

	/* Determine alignment and align */
	mi = new mi_xx(mo_align);
	switch (vdef->alignment()) {
	  case SIZE_OF_BYTE:			/* byte aligned */
	    mi->append_operand(immed(0));
	    break;
	  case SIZE_OF_HALFWORD:		/* halfword aligned */
	    mi->append_operand(immed(1));
	    break;
	  case SIZE_OF_WORD:			/* word aligned */
	    /* mips defaults int alignment to double-word alignment */
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
		("mips_print_var_def() -- bad alignment for %s",
		 vsym->name()));
	}
	mi->print(o_fd); delete mi;

	/* Disable automatic alignment */
	mi = new mi_xx(mo_align,immed(0));
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
			    mi = new mi_xx(mo_ascii, immed(data_string));
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
		    mi = new mi_xx(mo_ascii, immed(data_string));
		    mi->print(o_fd);
		    delete mi;

		} else 
		    assert_msg(FALSE,
			("mips_print_var_def() -- "
			 "k_multi-init value size = %d", value_size));

	    } else if (an->name() == k_repeat_init) {
		int repeat_cnt = (*il)[0].integer();
		int value_size = (*il)[1].integer();
		immed ivalue = (*il)[2];

		if (repeat_cnt > 1) {	/* insert .repeat pseudo-op */
		    mi = new mi_xx(mo_repeat,immed(repeat_cnt));
		    mi->print(o_fd); delete mi;
		}

		/* actual data op */
		switch (value_size) {		/* to get value size */
		  case SIZE_OF_BYTE:
		    mi = new mi_xx(mo_byte, ivalue);
		    break;
		  case SIZE_OF_HALFWORD:
		    mi = new mi_xx(mo_half, ivalue);
		    break;
		  case SIZE_OF_WORD:
		    if (ivalue.is_integer() || ivalue.is_ext_integer())
			mi = new mi_xx(mo_word, ivalue);
		    else if (ivalue.is_flt() || ivalue.is_ext_flt())
			/* got a FP single */
			mi = new mi_xx(mo_float, ivalue);
		    else if (ivalue.is_symbol())
			mi = new mi_xx(mo_word, ivalue);
		    else
			assert_msg(FALSE,
			    ("mips_print_var_def() -- "
			     "bad k_repeat_init value"));
		    break;
		  case SIZE_OF_DOUBLE:
		    if (ivalue.is_flt() || ivalue.is_ext_flt())
			mi = new mi_xx(mo_double, ivalue);
		     break;
		  default:
		    assert_msg(FALSE,
		        ("mips_print_var_def() -- "
			 "bad size for k_repeat_init"));
		}
		mi->print(o_fd); delete mi;

		if (repeat_cnt > 1) {	/* insert .endr pseudo-op */
		    mi = new mi_xx(mo_endr);
		    mi->print(o_fd); delete mi;
		}

	    } else if (an->name() == k_fill) {
		/* fill size with bit pattern, assuming size is a multiple
		 * of bytes and the bit pattern is all zeros. */
		int value_size = (*il)[0].integer();
		assert((value_size & 7) == 0);
		assert((*il)[1].integer() == 0);

		/* insert .repeat pseudo-op */
		mi = new mi_xx(mo_repeat,
			       immed(value_size >> SHIFT_BITS_TO_BYTES));
		mi->print(o_fd); delete mi;

		/* insert .byte 0 */
		mi = new mi_xx(mo_byte,(*il)[1]);
		mi->print(o_fd); delete mi;

		/* insert .endr pseudo-op */
		mi = new mi_xx(mo_endr);
		mi->print(o_fd); delete mi;

	    } //else if (an->name() == k_gprel_init) {
		/* mips-specific initialization -- generate a .gprel32
		 * pseudo-op for the var_sym.  Format of annotation is
		 * value_size followed by symbol whose offset we want. */
		//int value_size = (*il)[0].integer();
		//assert(value_size == 32);	/* only recognized size */
		/*assert(il->count() == 2);
		immed avalue = (*il)[1];
		mi = new mi_xx(mo_gprel32, avalue);
		mi->print(o_fd); delete mi;

	    }*/ else
	        warning_line(NULL, "mips_print_var_def() -- %s (%s)"
		        "ignoring unknown var_def annote", an->name());
	}

    } else {				/* uninitialized data item */
	/* if global symbol, append to .comm, else static symbol to .lcomm */
	if (vsym->is_global() && !vsym->is_private())
	    mi = new mi_xx(mo_comm, immed(vsym));
	else
	    mi = new mi_xx(mo_lcomm, immed(vsym));
	mi->append_operand(immed(vsym->type()->size() >> SHIFT_BITS_TO_BYTES));
	mi->print(o_fd); delete mi;
   }
}


/* mips_print_proc_def() -- */
void
mips_print_proc_def(proc_sym *p, FILE *o_fd)
{
    /* do nothing */
}


/* mips_print_proc_begin() -- Starts a procedure text segment. */
void
mips_print_proc_begin(proc_sym *psym, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(mo_text);				/* .text */
    mi->print(o_fd);
    delete mi;
    mi = new mi_xx(mo_align,immed(2));				/* .align 2 */
    mi->print(o_fd);
    delete mi;
}


/* mips_print_proc_entry() -- Reads the symbol table information on
 * the proc_sym and the annotations off the tree_proc.  It creates the
 * correct mips instructions to start a procedure text segment. */
void
mips_print_proc_entry(proc_sym *psym, int file_no_for_1st_line, FILE *o_fd)
{
    mi_xx *mi;
    mi = new mi_xx(mo_align,immed(2));				/* .align 2 */
    mi->print(o_fd);
    delete mi;

    /* if global procedure, then add this pseudo-op.  I.e. procedure is
     * in global symbol table, not in the file symbol table. */
    if (psym->is_global() && !psym->is_private()) {
	mi = new mi_xx(mo_globl,immed(psym->name()));		/* .globl */
	mi->print(o_fd);
	delete mi;
    }

    /* get file and line no. info for this procedure -- .file annote
     * already generated (if necessary) in Process_file() */
    immed_list *iml = (immed_list *)psym->block()->peek_annote(k_line);
    assert(iml && (strcmp((*iml)[1].string(),"") != 0));
    int first_line_no = (*iml)[0].integer();
    mi = new mi_xx(mo_loc,immed(file_no_for_1st_line));		/* .loc */
    mi->append_operand(immed(first_line_no));
    mi->print(o_fd);
    delete mi;

    mi = new mi_xx(mo_ent,immed(psym->name()));			/* .ent */
    mi->print(o_fd);
    delete mi;

    /* the procedure symbol cannot be a label symbol since SUIF does not
     * allow label symbols outside of a procedural context (this is the
     * reason why this routine exists in the first place), and so, we
     * have to generate the procedure label in a funny way. */
    fprintf(o_fd, "%s:\n", psym->name());			/* proc: */

    /* .frame (and others if not leaf) added to text during code gen */
}


/* mips_print_proc_end() -- outputs the .end pseudo-op. */
void
mips_print_proc_end(proc_sym *psym, FILE *o_fd)
{
    mi_xx *mi = new mi_xx(mo_end, immed(psym->name()));		/* .end */
    mi->print(o_fd);
    delete mi;
}
