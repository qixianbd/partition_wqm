/* file "machineUtil.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef MACHINE_UTIL_H
#define MACHINE_UTIL_H

extern char *which_architecture(mi_ops o);
extern int which_mformat(mi_ops o);

extern boolean Is_null(instruction *);
extern boolean Is_label(instruction *);
extern boolean Is_ldc(instruction *);
extern boolean Is_move(instruction *);
extern boolean Is_cmove(instruction *);         // conditional move
extern boolean Is_line(instruction *);          // line number marker

extern boolean Is_cti(instruction *);           // One of below is true
extern boolean Is_ubr(instruction *);           // unconditional jump/branch
extern boolean Is_cbr(instruction *);           // conditional (2 targets)
extern boolean Is_mbr(instruction *);           // multiway (n targets)
extern boolean Is_call(instruction *);          // includes coroutines calls
extern boolean Is_return(instruction *);

extern boolean Reads_memory(instruction *);
extern boolean Writes_memory(instruction *);

extern label_sym *Get_label(instruction *);     /* of label instr */
extern sym_node *Get_target(instruction *);     /* of branch/jump instr */
extern proc_sym *Get_proc(instruction *);       /* of call instr */

extern mi_ops Ubr_op(archinfo *);
extern mi_ops Label_op(archinfo *);
extern mi_ops Null_op(archinfo *);
extern mi_ops Load_op(archinfo *, type_node *);
extern mi_ops Store_op(archinfo *, type_node *);
extern mi_ops Move_op(archinfo *, type_node *);
extern mi_ops Invert_cbr_op(archinfo *, mi_ops);

extern void Print_global_directives(archinfo *, file_set_entry *, FILE *);
extern void Print_extern_op(archinfo *, var_sym *, FILE *);
extern void Print_file_op(archinfo *, int, char *, FILE *);
extern void Print_var_def(archinfo *, var_sym *, int, FILE *);
extern void Print_proc_def(archinfo *, proc_sym *, FILE *);
extern void Print_proc_begin(archinfo *, proc_sym *,  FILE *);
extern void Print_proc_entry(archinfo *, proc_sym *, int, FILE *);
extern void Print_proc_end(archinfo *, proc_sym *, FILE *);

#endif
