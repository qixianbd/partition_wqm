/* file "alphaInstr.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef ALPHA_INSTR_H
#define ALPHA_INSTR_H

class immed_list;
class type_node;
class instruction;
class machine_instr;
class mi_lab;
class mi_bj;
class mi_rr;
class mi_xx;

EXPORTED_BY_MACHINE char *k_alpha;

EXPORTED_BY_MACHINE char *k_gprel_init;

extern void init_alpha();

extern boolean alpha_is_ldc(instruction *);
extern boolean alpha_is_move(instruction *);
extern boolean alpha_is_cmove(instruction *);
extern boolean alpha_is_line(instruction *);
extern boolean alpha_is_ubr(instruction *);
extern boolean alpha_is_cbr(instruction *);
extern boolean alpha_is_call(instruction *);
extern boolean alpha_is_return(instruction *);
extern boolean alpha_reads_memory(instruction *);

extern mi_ops alpha_load_op(type_node *);
extern mi_ops alpha_store_op(type_node *);
extern mi_ops alpha_move_op(type_node *);

extern void alpha_lab_print(mi_lab *, FILE *);
extern void alpha_bj_print(mi_bj *, FILE *);
extern void alpha_rr_print(mi_rr *, FILE *);
extern void alpha_xx_print(mi_xx *, immed_list *, FILE *);
extern void alpha_print_global_directives(file_set_entry *, FILE *);
extern void alpha_print_extern_op(var_sym *, FILE *);
extern void alpha_print_file_op(int, char *, FILE *);
extern void alpha_print_var_def(var_sym *, int, FILE *);
extern void alpha_print_proc_def(proc_sym *, FILE *);
extern void alpha_print_proc_begin(proc_sym *, FILE *);
extern void alpha_print_proc_entry(proc_sym *, int, FILE *);
extern void alpha_print_proc_end(proc_sym *, FILE *);
extern void alpha_print_operand(operand *, FILE *);

#endif
