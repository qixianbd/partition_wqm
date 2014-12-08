/* file "machineUtil.cc" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

/*  This file contains all of the machine-specific dispatch routines,
 *  including routines that might have made more sense in other modules.
 *  When adding a new architecture, this is the **ONLY** non-architecture-
 *  specific module in the machine library that you will need to update.
 *  Furthermore, you should **not** need to change any non-architecture-
 *  specific pass. */

#include <suif_copyright.h>

#include <string.h>
#include <suif1.h>
#include "machine_internal.h"


/* init_machine() -- Stuff that should be done before any
 * machine-dependent pass. */
void
init_machine(int & /* argc */, char * /* argv */ [])
{
    k_null_string = lexicon->enter("")->sp;

    ANNOTE(k_comment,		"comment",		TRUE);
    ANNOTE(k_machine_instr,	"machine_instr",	TRUE);
    ANNOTE(k_hint,		"hint",			TRUE);
    ANNOTE(k_reloc,		"reloc",		TRUE);
    ANNOTE(k_instr_op_exts,	"instr_op_exts",	TRUE);
    ANNOTE(k_instr_xx_sources,	"instr_xx_sources",	TRUE);
    ANNOTE(k_instr_bj_target,	"instr_bj_target",	TRUE);
    ANNOTE(k_instr_mbr_tgts,	"instr_mbr_tgts",	TRUE);
    ANNOTE(k_instr_ret,		"instr_ret",		TRUE);
    ANNOTE(k_incomplete_proc_exit,"incomplete_proc_exit",TRUE);
    ANNOTE(k_mbr_index_def,	"mbr_index_def",	TRUE);
    ANNOTE(k_regs_used,		"regs_used",		TRUE);
    ANNOTE(k_regs_defd,		"regs_defd",		TRUE);
    ANNOTE(k_is_store_ea,	"is_store_ea",		TRUE);
    ANNOTE(k_proc_entry,	"proc_entry",		TRUE);
    ANNOTE(k_target_arch,	"target_arch",		TRUE);
    ANNOTE(k_next_free_reloc_num,"next_free_reloc_num",	TRUE);
    ANNOTE(k_vreg_manager,	"vreg_manager",		TRUE);
    ANNOTE(k_stack_frame_info,	"stack_frame_info",	TRUE);
    ANNOTE(k_vsym_info,		"vsym_info",		TRUE);
    ANNOTE(k_header_trailer,	"header/trailer",	TRUE);

    // initialize the nonprinting_annotes list
    nonprinting_annotes = new nonprinting_annotes_list();
    nonprinting_annotes->append(k_hint);
    nonprinting_annotes->append(k_instr_op_exts);
    nonprinting_annotes->append(k_is_store_ea);
    nonprinting_annotes->append(k_reloc);

    /* include all architecture-specific start stuff here */
#ifdef M_ALPHA
    init_alpha();
#endif
#ifdef M_MIPS
    init_mips();
#endif
#ifdef M_PPC
    init_ppc(); 
#endif
#ifdef M_X86
    init_x86();
#endif
}

void
exit_machine(void)
{
    delete nonprinting_annotes;
    return;
}


/* ------------ archinfo methods ---------------- */

/* archinfo::init_from_annote() -- read architecture-specific target
 * information from k_target_arch annote on file_set_entry. 
 * If there is no such annotation, then we must be in low SUIF.  */
void
archinfo::init_from_annote(file_set_entry *fse)
{
    immed_list *iml;
    iml = (immed_list *)fse->peek_annote(k_target_arch);

    if (iml == NULL) {
	fam = k_suif; 
	ver = "1";
	vos = "stanford-generic";
	impl = "unknown"; 
    } else {
	fam = (*iml)[0].string();
	ver = (*iml)[1].string();
	vos = (*iml)[2].string();
	impl = (*iml)[3].string();
    }

    #ifdef M_ALPHA 
    if (!strcmp(fam, k_alpha)) {
	first = (if_ops)ao_FIRST_OP; 
	last = (if_ops)ao_LAST_OP; 
    }
    #endif
    #ifdef M_MIPS 
    if (!strcmp(fam, k_mips)) {
	first = (if_ops)mo_FIRST_OP; 
	last = (if_ops)mo_LAST_OP; 
    }
    #endif
    #ifdef M_PPC 
    if (!strcmp(fam, k_ppc)) {
	first = (if_ops)po_FIRST_OP; 
	last = (if_ops)po_LAST_OP; 
    }
    #endif
    #ifdef M_X86 
    if (!strcmp(fam, k_x86)) {
	first = (if_ops)xo_FIRST_OP; 
	last = (if_ops)xo_LAST_OP; 
    }
    #endif
}


/* archinfo::op_string() -- return the string for a given opcode
 * based on the architecture. */
char *
archinfo::op_string(if_ops opcode)
{
    if (io_eos <= opcode && opcode < io_last) 
	return if_ops_name(opcode);

#ifdef M_ALPHA
    if ((if_ops)ao_FIRST_OP <= opcode && opcode < (if_ops)ao_LAST_OP)
	return alpha_op_string(opcode);
#endif
#ifdef M_MIPS
    if ((if_ops)mo_FIRST_OP <= opcode && opcode < (if_ops)mo_LAST_OP)
	return mips_op_string(opcode);
#endif
#ifdef M_PPC
    if ((if_ops)po_FIRST_OP <= opcode && opcode < (if_ops)po_LAST_OP)
	return ppc_op_string(opcode);
#endif
#ifdef M_X86
    if ((if_ops)xo_FIRST_OP <= opcode && opcode < (if_ops)xo_LAST_OP)
	return x86_op_string(opcode);
#endif

    assert_msg(FALSE, ("op_string() -- unknown opcode %d", opcode));

    return NULL;		/* to keep compiler quiet */
}


/* ------------ machine_instr methods -------------- */

/* machine_instr::op_string() -- return the string for a given opcode
 * based on the architecture. */
char *
machine_instr::op_string()
{
    if (architecture() == k_suif) {
	if (m_op == io_null) return (k_null_string);
	else return if_ops_name((if_ops)m_op);
    }
#ifdef M_ALPHA
    else if (architecture() == k_alpha) return alpha_op_string(m_op);
#endif
#ifdef M_MIPS
    else if (architecture() == k_mips) return mips_op_string(m_op);
#endif
#ifdef M_PPC
    else if (architecture() == k_ppc) return ppc_op_string(m_op);
#endif
#ifdef M_X86
    else if (architecture() == k_x86) return x86_op_string(m_op);
#endif

    assert_msg(FALSE, ("op_string() -- unknown architecture %s",
		       architecture()));

    return NULL;		/* to keep compiler quiet */
}


/* ------------ mi_* print helper routines -------------- */

/* mi_lab::print() -- print out instruction of mif_lab format by calling
 * appropriate architecture specific print routine. */
void
mi_lab::print(FILE *o_fd)
{
#ifdef M_ALPHA
    if (architecture() == k_alpha) {
	alpha_lab_print(this, o_fd);
	return;
    }
#endif
#ifdef M_MIPS
    if (architecture() == k_mips) {
	mips_lab_print(this, o_fd);
	return;
    }
#endif
#ifdef M_PPC
    if (architecture() == k_ppc) {
	ppc_lab_print(this, o_fd);
	return;
    }
#endif
#ifdef M_X86
    if (architecture() == k_x86) {
	x86_lab_print(this, o_fd);
	return;
    }
#endif
    assert_msg(FALSE, ("architecture() unset or mi_lab::print() "
		       "function incomplete"));
}


/* mi_bj::print() -- print out instruction of mif_bj format by calling
 * appropriate architecture specific print routine. */
void
mi_bj::print(FILE *o_fd)
{
#ifdef M_ALPHA
    if (architecture() == k_alpha) {
	alpha_bj_print(this, o_fd);
	return;
    }
#endif
#ifdef M_MIPS
    if (architecture() == k_mips) {
	mips_bj_print(this, o_fd);
	return;
    }
#endif
#ifdef M_PPC
    if (architecture() == k_ppc) {
	ppc_bj_print(this, o_fd);
	return;
    }
#endif
#ifdef M_X86
    if (architecture() == k_x86) {
	x86_bj_print(this, o_fd);
	return;
    }
#endif
    assert_msg(FALSE, ("architecture() unset or mi_bj::print() "
		       "function incomplete"));
}


/* mi_rr::print() -- print out instruction of mif_rr format by calling
 * appropriate architecture specific print routine. */
void
mi_rr::print(FILE *o_fd)
{
#ifdef M_ALPHA
    if (architecture() == k_alpha) {
	alpha_rr_print(this, o_fd);
	return;
    }
#endif
#ifdef M_MIPS
    if (architecture() == k_mips) {
	mips_rr_print(this, o_fd);
	return;
    }
#endif
#ifdef M_PPC
    if (architecture() == k_ppc) {
	ppc_rr_print(this, o_fd);
	return;
    }
#endif
#ifdef M_X86
    if (architecture() == k_x86) {
	x86_rr_print(this, o_fd);
	return;
    }
#endif
    assert_msg(FALSE, ("architecture() unset or mi_rr::print() "
		       "function incomplete"));
}

/* mi_xx::print() -- print out instruction of mif_xx format by calling
 * appropriate architecture specific print routine. */
void
mi_xx::print(FILE *o_fd)
{
#ifdef M_ALPHA
    if (architecture() == k_alpha) {
	alpha_xx_print(this, &il, o_fd);
	return;
    }
#endif
#ifdef M_MIPS
    if (architecture() == k_mips) {
	mips_xx_print(this, &il, o_fd);
	return;
    }
#endif
#ifdef M_PPC
    if (architecture() == k_ppc) {
	ppc_xx_print(this, &il, o_fd);
	return;
    }
#endif
#ifdef M_X86
    if (architecture() == k_x86) {
	x86_xx_print(this, &il, o_fd);
	return;
    }
#endif
    assert_msg(FALSE, ("architecture() unset or mi_xx::print() "
		       "function incomplete"));
}


/* ------------ Query helper routines -------------- */

/* which_architecture() -- return the character string corresponding
 * to this opcode's architecture. */
char *
which_architecture(mi_ops o)
{
    if ((o > 0) && (o < io_last)) return k_suif;
#ifdef M_ALPHA
    else if ((o > ao_FIRST_OP) && (o < ao_LAST_OP)) return k_alpha;
#endif
#ifdef M_MIPS
    else if ((o > mo_FIRST_OP) && (o < mo_LAST_OP)) return k_mips;
#endif
#ifdef M_PPC
    else if ((o > po_FIRST_OP) && (o < po_LAST_OP)) return k_ppc; 
#endif
#ifdef M_X86
    else if ((o > xo_FIRST_OP) && (o < xo_LAST_OP)) return k_x86;
#endif

    assert_msg(FALSE, ("opcode %d out-of-range or which_architecture() "
		       "function incomplete", o));

    return k_null_string;	/* keep compiler quiet */
}

/* which_mformat() -- return the format for any machine opcode. */
int
which_mformat(mi_ops o)
{
    if ((o > 0) && (o < io_last))
	return which_format((if_ops)o);
#ifdef M_ALPHA
    else if ((o > ao_FIRST_OP) && (o < ao_LAST_OP))
	return alpha_which_mformat(o);
#endif
#ifdef M_MIPS
    else if ((o > mo_FIRST_OP) && (o < mo_LAST_OP))
	return mips_which_mformat(o);
#endif
#ifdef M_PPC
    else if ((o > po_FIRST_OP) && (o < po_LAST_OP))
	return ppc_which_mformat(o);
#endif
#ifdef M_X86
    else if ((o > xo_FIRST_OP) && (o < xo_LAST_OP))
	return x86_which_mformat(o);
#endif

    assert_msg(FALSE, ("opcode %d out-of-range or which_mformat() "
		       "function incomplete", o));

    return 0;			/* keep compiler quiet */
}


/* Is_null() -- returns TRUE if null instruction.  Notice that we don't
 * check in this routine to see if you've got the correct helper function. */
boolean
Is_null(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_null) return TRUE;		/* SUIF */
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return(op == ao_null);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return(op == mo_null);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return(op == po_null);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return(op == xo_null);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_null() -- "
	"unknown architecture %s", i->architecture()));

    return FALSE;		/* SUIF default */
}

/* Is_label() -- return TRUE if label instruction.  Notice that we don't
 * check in this routine to see if you've got the correct helper function. */
boolean
Is_label(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_lab) return TRUE;		/* SUIF */
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return(op == ao_lab);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return(op == mo_lab);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return(op == po_lab);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return(op == xo_lab);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_label() -- "
	"unknown architecture %s", i->architecture()));

    return FALSE;		/* SUIF default */
}

/* Is_ldc() -- return TRUE if load-constant instruction.  Do NOT include
 * load-address opcodes; they typically use memory formats. */
boolean
Is_ldc(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_ldc) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_ldc(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_ldc(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_ldc(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_ldc(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_ldc() -- "
	"unknown architecture %s", i->architecture()));

    return FALSE;		/* SUIF default */
}

/* Is_move() -- returns TRUE if instruction is a simple move operation. */
boolean
Is_move(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_cpy) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_move(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_move(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_move(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_move(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_move() -- "
        "unknown architecture %s", i->architecture()));

    return FALSE;		/* SUIF default */
}

/* Is_cmove() -- returns TRUE if instruction is a conditional
 * move operation. */
boolean
Is_cmove(instruction *i)
{
    int op = (int)i->opcode();
    if (FALSE) {
	// no conditional move instruction in low-suif
    }
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_cmove(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_cmove(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_cmove(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_cmove(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_cmove() -- "
        "unknown architecture %s", i->architecture()));

    return FALSE;		/* SUIF default */
}

/* Is_line() -- returns TRUE if instruction represents a line
 * number marker. */
boolean
Is_line(instruction *i)
{
    int op = (int)i->opcode();
    if (i->architecture() == k_suif)
	return ((op == io_mrk) && i->peek_annote(k_line));
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha)
	return alpha_is_line(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips)
	return mips_is_line(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc)
	return ppc_is_line(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86)
	return x86_is_line(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_line() -- "
        "unknown architecture %s", i->architecture()));

    return FALSE;		/* SUIF default */
}

/* Is_cti() -- return TRUE if control-transfer instruction 
 * This implementation is slow, but is guaranteed to give
 * the correct semantics. 
 */
boolean Is_cti(instruction *i)
{
    return Is_ubr(i) || Is_cbr(i) || Is_mbr(i) 
	|| Is_call(i) || Is_return(i); 
}

/* Is_ubr() -- return TRUE if instruction is an unconditional
 * jump or branch. 
 */
boolean
Is_ubr(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_jmp) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_ubr(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_ubr(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_ubr(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_ubr(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_ubr() -- "
	"unknown architecture %s", i->architecture()));
    return FALSE;		/* SUIF default */
}

/* Is_cbr() -- return TRUE if instruction uses conditional
 * branch machine resources (independent of whether the branch
 * evaluation is conditional or not). */
boolean
Is_cbr(instruction *i)
{
    int op = (int)i->opcode();
    if ((op == io_btrue) || (op == io_bfalse)) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_cbr(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_cbr(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_cbr(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_cbr(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_cbr() -- "
	"unknown architecture %s", i->architecture()));
    return FALSE;		/* SUIF default */
}

/* Is_mbr() -- return TRUE if instruction is a multi-way branch. */
boolean
Is_mbr(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_mbr) 
	return TRUE;
    else if (FALSE
#ifdef M_ALPHA
	     || (i->architecture() == k_alpha)
#endif
#ifdef M_MIPS
	     || (i->architecture() == k_mips) 
#endif
#ifdef M_PPC
	     || (i->architecture() == k_ppc) 
#endif
#ifdef M_X86
	     || (i->architecture() == k_x86)
#endif
	    )
	return i->peek_annote(k_instr_mbr_tgts) != NULL; 

    assert_msg((i->architecture() == k_suif), ("Is_mbr() -- "
	"unknown architecture %s", i->architecture()));
    return FALSE;		/* SUIF default */
}

/* Is_call() -- return TRUE if instruction is a call. */
boolean
Is_call(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_cal) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_call(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_call(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_call(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_call(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_call() -- "
	"unknown architecture %s", i->architecture()));
    return FALSE;		/* SUIF default */
}

/* Is_return() -- return TRUE if instruction is a return. */
boolean
Is_return(instruction *i)
{
    int op = (int)i->opcode();
    if (op == io_ret) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_is_return(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_is_return(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_is_return(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_is_return(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Is_return() -- "
	"unknown architecture %s", i->architecture()));
    return FALSE;		/* SUIF default */
}

/* Reads_memory() -- return TRUE of any source operand located in memory.
 * This routine calls machine-specific helpers since some instructions
 * with base+offset as an operand do not read memory. */
boolean
Reads_memory(instruction *i)
{
    int op = (int)i->opcode();
    if ((op == io_lod) || (op == io_memcpy)) return TRUE;
#ifdef M_ALPHA
    else if (i->architecture() == k_alpha) return alpha_reads_memory(i);
#endif
#ifdef M_MIPS
    else if (i->architecture() == k_mips) return mips_reads_memory(i);
#endif
#ifdef M_PPC
    else if (i->architecture() == k_ppc) return ppc_reads_memory(i);
#endif
#ifdef M_X86
    else if (i->architecture() == k_x86) return x86_reads_memory(i);
#endif

    assert_msg((i->architecture() == k_suif), ("Reads_memory() -- "
	"unknown architecture %s", i->architecture()));
    return FALSE;		/* SUIF default */
}

/* Writes_memory() -- return TRUE of the operation writes memory, e.g.
 * a store instruction. */
boolean
Writes_memory(instruction *i)
{
    if (i->architecture() == k_suif) {		/* suif instr */
	int op = (int)i->opcode();
	if ((op == io_str) || (op == io_memcpy)) return TRUE;

    } else {					/* machine instr */
	int fmt = (int)i->format();
	if (((fmt == mif_rr) || (fmt == mif_bj))
	&& i->peek_annote(k_is_store_ea))
	    return TRUE;
    }

    return FALSE;
}



/* ------------ Access helper routines -------------- */

/* Get_label() -- helper routine that performs the appropriate cast
 * to get at the label_sym on a label instruction. */
label_sym *
Get_label(instruction *i)
{
    assert(Is_label(i));
    if (i->architecture() == k_suif)
	return ((in_lab *)i)->label();
    else
	return ((mi_lab *)i)->label();
}


/* Get_target() -- helper routine that performs the appropriate cast
 * to get at the target sym_node on a branch/jump instruction.  Note
 * that the machine_instr may return NULL if it is an mbr instruction. */
sym_node *
Get_target(instruction *i)
{
    assert(Is_cti(i));
    if (i->architecture() == k_suif)
	return ((in_bj *)i)->target();
    else
	return ((mi_bj *)i)->target();
}


/* Get_proc() -- helper routine that traverses the different kinds of
 * call instructions to get at the proc_sym of the called procedure. 
 * Returns NULL on calls to function variables. */
proc_sym *
Get_proc(instruction *i)
{
    assert(Is_call(i)); 
    if (i->architecture() == k_suif) {
	assert(i->opcode() == io_cal); 
	in_cal *call = (in_cal *)i; 
	operand addr = call->addr_op(); 
	if (!addr.is_instr() || !addr.instr()->format() == inf_ldc)
	    return NULL; 
	in_ldc *ldc = (in_ldc *)addr.instr(); 
	immed im = ldc->value(); 
	if (!im.is_symbol() || !im.symbol()->is_proc())
	    return NULL; 
	return (proc_sym *)im.symbol(); 
    }
    else {
	mi_bj *call = (mi_bj *)i; 
	if (!call->target() || !call->target()->is_proc())
	    return NULL; 
	return (proc_sym *)call->target(); 
    }
}


/* ------------ Code generation helper routines -------------- */

/* Ubr_op() -- helper routine that returns the architecture-specific
 * opcode for an unconditional branch/jump. */
mi_ops
Ubr_op(archinfo *tgt)
{
    if (tgt->family() == k_suif) return io_jmp;
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha) return ao_br;
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips) return mo_j;
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc) return po_b;
#endif
    else assert_msg(FALSE, ("Ubr_op() -- unknown architecture"));

    return io_null;	/* should never reach this point */
}

/* Label_op() -- helper routine that returns an architecture-specific
 * label opcode. */
mi_ops
Label_op(archinfo *tgt)
{
    if (tgt->family() == k_suif) return io_lab;
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha) return ao_lab;
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips) return mo_lab;
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc) return po_lab;
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86) return xo_lab;
#endif
    else assert_msg(FALSE, ("Label_op() -- unknown architecture"));

    return io_null;	/* should never reach this point */
}


/* Null_op() -- helper routine that returns an architecture-specific
 * null opcode. */
mi_ops
Null_op(archinfo *tgt)
{
    if (tgt->family() == k_suif) return io_null;
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha) return ao_null;
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips) return mo_null;
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc) return po_null;
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86) return xo_null;
#endif
    else assert_msg(FALSE, ("Null_op() -- unknown architecture"));

    return io_null;	/* should never reach this point */
}


/* Load_op() -- helper routine that returns the appropriate load opcode
 * for the specified SUIF type.  Routine works for INT, FLOAT, VOID, PTR,
 * and ENUM type_ops. */
mi_ops
Load_op(archinfo *tgt, type_node *typ)
{
    if (tgt->family() == k_suif) return io_lod;
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha) return alpha_load_op(typ);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips) return mips_load_op(typ);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc) return ppc_load_op(typ);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86) return x86_load_op(typ);
#endif
    else assert_msg(FALSE, ("Load_op() -- unknown architecture"));

    return io_null;	/* should never reach this point */
}

/* Store_op() -- helper routine that returns the appropriate store opcode
 * for the specified SUIF type.  Routine works for INT, FLOAT, VOID, PTR,
 * and ENUM type_ops. */
mi_ops
Store_op(archinfo *tgt, type_node *typ)
{
    if (tgt->family() == k_suif) return io_str;
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha) return alpha_store_op(typ);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips) return mips_store_op(typ);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc) return ppc_store_op(typ);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86) return x86_store_op(typ);
#endif
    else assert_msg(FALSE, ("Store_op() -- unknown architecture"));

    return io_null;	/* should never reach this point */
}

/* Move_op() -- helper routine that returns the appropriate move opcode
 * for the specified SUIF type.  Routine works for INT, FLOAT, VOID, PTR,
 * and ENUM type_ops. */
mi_ops
Move_op(archinfo *tgt, type_node *typ)
{
    if (tgt->family() == k_suif) return io_str;
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha) return alpha_move_op(typ);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips) return mips_move_op(typ);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc) return ppc_move_op(typ);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86) return x86_move_op(typ);
#endif
    else assert_msg(FALSE, ("Move_op() -- unknown architecture"));

    return io_null;	/* should never reach this point */
}

/* Invert_cbr_op() -- architecture-specific helper routine that returns
 * the conditional branch opcode corresponding to the inversion of the
 * conditional branch passed as the 2nd parameter. */
mi_ops
Invert_cbr_op(archinfo *tgt, mi_ops orig_op)
{
    int *invert_table = NULL;

    if (tgt->family() == k_suif) {
	if (orig_op == io_btrue) return io_bfalse;
	else if (orig_op == io_bfalse) return io_btrue;
	else assert_msg(FALSE, ("Invert_cbr_op() -- unexpected CBR op %s",
				orig_op));
#ifdef M_ALPHA
    } else if (tgt->family() == k_alpha) {
	invert_table = alpha_invert_table;
#endif
#ifdef M_MIPS
    } else if (tgt->family() == k_mips) {
	invert_table = mips_invert_table;
#endif
#ifdef M_PPC
    } else if (tgt->family() == k_ppc) {
	invert_table = ppc_invert_table;
#endif
#ifdef M_X86
    } else if (tgt->family() == k_x86) {
	invert_table = x86_invert_table;
#endif
    }

    assert_msg(invert_table, ("Invert_cbr_op() -- unknown architecture"));

    /* walk indicated table */
    int i;
    for (i = 0; invert_table[i] != -1; i += 2)
	if ((invert_table[i] == orig_op) || (invert_table[i + 1] == orig_op))
	    break;

    assert_msg(invert_table[i + 1] != -1, 
	("Invert_cbr_op() -- unsupported opcode %d", orig_op)); 

    mi_ops new_op = (invert_table[i] == orig_op)
	? invert_table[i + 1]
     	: invert_table[i]; 

    return new_op;
}


/* ------------ More print helper routines -------------- */

/* Print_global_directives() -- sometimes we need to insert assembler
 * directives at the top of the assembly-language file that are valid
 * for the entire file.  This routine implements this functionality.  We
 * assume that you attached the information to the file_set_entry. */
void
Print_global_directives(archinfo *tgt, file_set_entry *fse, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_global_directives() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_global_directives(fse, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_global_directives(fse, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	assert_msg(FALSE, ("Print_global_directives() -- "
			   "unimplemented code, please fix."));
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_global_directives(fse, o_fd);
#endif
    else assert_msg(FALSE, ("Print_global_directives() -- "
			    "unknown architecture"));
}


/* Print_extern_op() -- print out the pseudo ops required for
 * extern symbols. */
void
Print_extern_op(archinfo *tgt, var_sym *v, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_extern_op() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_extern_op(v, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_extern_op(v, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_extern_op(v, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_extern_op(v, o_fd);
#endif
    else assert_msg(FALSE, ("Print_extern_op() -- unknown architecture"));
}


/* Print_file_op() -- print out the source file directive. */
void
Print_file_op(archinfo *tgt, int fnum, char *fnam, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_file_op() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_file_op(fnum, fnam, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_file_op(fnum, fnam, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_file_op(fnum, fnam, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_file_op(fnum, fnam, o_fd);
#endif
    else assert_msg(FALSE, ("Print_file_op() -- unknown architecture"));
}


/* Print_var_def() -- print out the pseudo ops required for
 * this static variable definition. */
void
Print_var_def(archinfo *tgt, var_sym *v, int Gnum, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_var_def() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_var_def(v, Gnum, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_var_def(v, Gnum, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_var_def(v, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_var_def(v, o_fd);
#endif
    else assert_msg(FALSE, ("Print_var_def() -- unknown architecture"));
}


/* Print_proc_def() -- print out the pseudo ops required for
 * a procedure definition.  The ppc architecture is the only
 * architecture to use this function at this time. */
void
Print_proc_def(archinfo *tgt, proc_sym *p, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_proc_def() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_proc_def(p, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_proc_def(p, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_proc_def(p, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_proc_def(p, o_fd);
#endif
    else assert_msg(FALSE, ("Print_proc_def() -- unknown architecture"));
}


/* Print_proc_begin() -- print out the pseudo ops required to start
 * the text segment of a procedure. */
void
Print_proc_begin(archinfo *tgt, proc_sym *p, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_proc_begin() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_proc_begin(p, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_proc_begin(p, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_proc_begin(p, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_proc_begin(p, o_fd);
#endif
    else assert_msg(FALSE, ("Print_proc_begin() -- unknown architecture"));
}


/* Print_proc_entry() -- print out the pseudo ops required at the entry
 * point of a procedure.  Note that the entry point does not have to
 * be at the start of the procedure's text segment.  For debugging,
 * we create a line directive at the beginning of the procedure.  To do
 * this in most architectures, we require the file indentifer fnum. */
void
Print_proc_entry(archinfo *tgt, proc_sym *p, int fnum, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_proc_entry() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_proc_entry(p, fnum, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_proc_entry(p, fnum, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_proc_entry(p, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_proc_entry(p, fnum, o_fd);
#endif
    else assert_msg(FALSE, ("Print_proc_entry() -- unknown architecture"));
}


/* Print_proc_end() -- print out the pseudo ops required to end
 * the text segment of a procedure. */
void
Print_proc_end(archinfo *tgt, proc_sym *p, FILE *o_fd)
{
    if (tgt->family() == k_suif)
	assert_msg(FALSE, ("Print_proc_end() -- "
			   "how did you want this printed?"));
#ifdef M_ALPHA
    else if (tgt->family() == k_alpha)
	alpha_print_proc_end(p, o_fd);
#endif
#ifdef M_MIPS
    else if (tgt->family() == k_mips)
	mips_print_proc_end(p, o_fd);
#endif
#ifdef M_PPC
    else if (tgt->family() == k_ppc)
	ppc_print_proc_end(p, o_fd);
#endif
#ifdef M_X86
    else if (tgt->family() == k_x86)
	x86_print_proc_end(p, o_fd);
#endif
    else assert_msg(FALSE, ("Print_proc_end() -- unknown architecture"));
}

