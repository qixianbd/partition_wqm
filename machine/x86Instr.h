/* file "x86Instr.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1996-1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*
 * This file describes the data structure representing a x86 assembly
 * language registers, symbols, instructions, and pseudo-operations.
 *
 * Independent of how the assembly language instruction is printed out,
 * the destinations must reside in dest array and the source operands in
 * src array.  If not, things will break.  Those instructions that x86 prints
 * in any strange way are handled in the print routine.
 */


#ifndef X86_INSTR_H
#define X86_INSTR_H


class immed_list;
class type_node;
class instruction;
class machine_instr;
class mi_lab;
class mi_bj;
class mi_rr;
class mi_xx;


/*
 *  Useful string constants, types, and procedure declarations for
 *  x86 instruction set.
 */
EXPORTED_BY_MACHINE char *k_x86;

/* The following are macros for accessing x86 registers by a sensible
 * name.  The first four registers are considered as temporary
 * (caller-saved) registers.  There are no argument, return, or
 * callee-saved registers (except the base pointer). */

#define REG_eax REG(GPR,TMP,0)
#define REG_ecx REG(GPR,TMP,4)
#define REG_edx REG(GPR,TMP,8)
#define REG_ebx REG(GPR,TMP,12)
#define REG_esi REG(GPR,GEN,0)
#define REG_edi REG(GPR,GEN,4)

#define REG_eflags REG(CTL,GEN,0)

/* There are no x86 FP registers.  FP operations are handled by a
 * 8-entry, FP stack.  Stack indices in the REG structure are relative
 * to the current top-of-stack!  They do not represent the actual index
 * into the actual hardware structure. */
#define STACK_fp0 REG(FPR,TMP,0)	/* top of stack, always! */

#define REG_fpflags REG(CTL,GEN,3)	/* fp status register */

/* The following are macros to make it easy to build x86 instructions:
 *   NEW_x86 -- construct a x86 machine_instr with 2 destinations.
 */

#define NEW_x86(_mi, _class, _op, _d0, _d1, _s0, _s1) \
        _mi = new _class(_op, _d0, _s0, _s1); \
	_mi->set_num_dsts(2); \
	_mi->set_dst(1, _d1)

void init_x86();

extern boolean x86_is_ldc(instruction *);
extern boolean x86_is_move(instruction *);
extern boolean x86_is_cmove(instruction *);
extern boolean x86_is_line(instruction *);
extern boolean x86_is_cbr(instruction *);
extern boolean x86_is_ubr(instruction *);
extern boolean x86_is_call(instruction *);
extern boolean x86_is_return(instruction *);
extern boolean x86_reads_memory(instruction *);

extern mi_ops x86_load_op(type_node *);
extern mi_ops x86_store_op(type_node *);
extern mi_ops x86_move_op(type_node *);

extern void x86_lab_print(mi_lab *, FILE *);
extern void x86_bj_print(mi_bj *, FILE *);
extern void x86_rr_print(mi_rr *, FILE *);
extern void x86_xx_print(mi_xx *, immed_list *, FILE *);

extern void x86_print_global_directives(file_set_entry *, FILE *);
extern void x86_print_extern_op(var_sym *, FILE *);
extern void x86_print_file_op(int, char *, FILE *);
extern void x86_print_var_def(var_sym *, FILE *);
extern void x86_print_proc_def(proc_sym *, FILE *);
extern void x86_print_proc_begin(proc_sym *, FILE *);
extern void x86_print_proc_entry(proc_sym *, int, FILE *);
extern void x86_print_proc_end(proc_sym *, FILE *);

#endif
