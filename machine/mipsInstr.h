/* file "mipsInstr.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef MIPS_INSTR_H
#define MIPS_INSTR_H
class immed_list;
class type_node;
class instruction;
class machine_instr;
class mi_lab;
class mi_bj;
class mi_rr;
class mi_xx;

EXPORTED_BY_MACHINE char *k_mips;

//EXPORTED_BY_MACHINE char *k_gprel_init;

extern void init_mips();

extern boolean mips_is_ldc(instruction *);
extern boolean mips_is_move(instruction *);
extern boolean mips_is_cmove(instruction *);
extern boolean mips_is_line(instruction *);
extern boolean mips_is_ubr(instruction *);
extern boolean mips_is_cbr(instruction *);
extern boolean mips_is_call(instruction *);
extern boolean mips_is_return(instruction *);
extern boolean mips_reads_memory(instruction *);

extern mi_ops mips_load_op(type_node *);
extern mi_ops mips_store_op(type_node *);
extern mi_ops mips_move_op(type_node *);

extern void mips_lab_print(mi_lab *, FILE *);
extern void mips_bj_print(mi_bj *, FILE *);
extern void mips_rr_print(mi_rr *, FILE *);
extern void mips_xx_print(mi_xx *, immed_list *, FILE *);
extern void mips_print_global_directives(file_set_entry *, FILE *);
extern void mips_print_extern_op(var_sym *, FILE *);
extern void mips_print_file_op(int, char *, FILE *);
extern void mips_print_var_def(var_sym *, int, FILE *);
extern void mips_print_proc_def(proc_sym *, FILE *);
extern void mips_print_proc_begin(proc_sym *, FILE *);
extern void mips_print_proc_entry(proc_sym *, int, FILE *);
extern void mips_print_proc_end(proc_sym *, FILE *);
extern void mips_print_operand(operand *, FILE *);

#endif
