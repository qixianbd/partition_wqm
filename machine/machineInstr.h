/* file "machineInstr.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef MACHINE_INSTR_H
#define MACHINE_INSTR_H

EXPORTED_BY_MACHINE char *k_null_string;

/*
 *  Flat annotations used by machsuif passes.
 */
EXPORTED_BY_MACHINE char *k_comment;
EXPORTED_BY_MACHINE char *k_instr_op_exts;
EXPORTED_BY_MACHINE char *k_hint;
EXPORTED_BY_MACHINE char *k_reloc;

/* ... following are used during file I/O (attached to instructions) */
EXPORTED_BY_MACHINE char *k_machine_instr;
EXPORTED_BY_MACHINE char *k_instr_xx_sources;
EXPORTED_BY_MACHINE char *k_instr_bj_target;

/* ... following keep extra high-level info with instruction */
EXPORTED_BY_MACHINE char *k_instr_ret;
EXPORTED_BY_MACHINE char *k_incomplete_proc_exit;
EXPORTED_BY_MACHINE char *k_instr_mbr_tgts;
EXPORTED_BY_MACHINE char *k_mbr_index_def;
EXPORTED_BY_MACHINE char *k_regs_defd;
EXPORTED_BY_MACHINE char *k_regs_used;
EXPORTED_BY_MACHINE char *k_is_store_ea;

/* ... following are markers in text list (attached to *o_null instrs) */
EXPORTED_BY_MACHINE char *k_proc_entry;         /* marks entry point to proc */

/* ... following are attached to file_set_entry's */
EXPORTED_BY_MACHINE char *k_target_arch;        /* arch and machine info */
EXPORTED_BY_MACHINE char *k_next_free_reloc_num;

/* ... following are attached to tree_proc's */
EXPORTED_BY_MACHINE char *k_vreg_manager;       /* should be in proc_symtab */
EXPORTED_BY_MACHINE char *k_stack_frame_info;/* used by *gen, ra*, and *fin */

/* ... following are attached to sym_node's */
EXPORTED_BY_MACHINE char *k_vsym_info;  /* extra info for this var_sym */

/* ... following identify instructions added by *fin */
EXPORTED_BY_MACHINE char *k_header_trailer; /* header/trailer code/pseudo-op */

/*
 *  Helper functions related to the generic machine instruction class.
 */
extern void init_machine(int &argc, char *argv[]);
extern void exit_machine(void);

class mproc_symtab {
  private:
    proc_symtab *st;    /* where we'd like this info */
    int next_vrnum;     /* next unused virtual register number */

  public:
    mproc_symtab(proc_symtab *t)        { st = t; next_vrnum = -1; }
    ~mproc_symtab()                     {}

    proc_symtab *pst()                  { return st; }

    int vreg_num()                      { return next_vrnum; }
    int next_vreg_num()                 { return next_vrnum--; }
    void set_vreg_num(int n)            { assert(n < 0); next_vrnum = n; }

    void renumber_vregs();      /* renumbers vr's uses in tree_proc body */
};
 
extern mproc_symtab *Read_machine_proc(proc_sym *, boolean /* exp_trees */,
                                       boolean /* use_fortran_form */);
extern void Write_machine_proc(proc_sym *, file_set_entry *);

extern void Print_data_label(FILE *, sym_node *, char);
extern void Print_raw_immed(FILE *, immed);
extern void Print_symbol(FILE *, sym_node *);

EXPORTED_BY_MACHINE boolean skip_printing_of_unwanted_annotes;
DECLARE_LIST_CLASS(nonprinting_annotes_list, char *);
EXPORTED_BY_MACHINE nonprinting_annotes_list *nonprinting_annotes;
extern boolean Is_ea_operand(operand);
extern operand Immed_operand(immed &, type_node *);
extern void
Map_operand(instruction *in, operand (*fun)(operand, boolean, void *), void *x);

/*
 *  Machine instruction classes.
 */
class machine_instr: public in_gen {
    int m_op;                           /* architecture-specific opcode */
    /* opcode extensions are included as annotations */

  protected:
    /* The base SUIF instruction class defaults to: io_nop, type_void,
     * inum = 0, par = NULL, and dst = operand(). */
    machine_instr():in_gen(k_null_string, type_void, operand(), 0)
        { m_op = io_null; set_result_type(type_void); }
    machine_instr(char *a, int op):in_gen(a, type_void, operand(), 0)
        { m_op = op; set_result_type(type_void); }

    void clone_base(machine_instr *i, replacements *r, boolean no_copy); 

  public:
    ~machine_instr() {}

    /* normal assembly instruction information */
    char *architecture()                { return name(); }
    virtual inst_format format()        { return inf_none; }

    /* opcode stuff including ugly hack to override opcode return type */
    if_ops opcode()                     { return (if_ops)m_op; }
    char *op_string();
    void set_opcode(if_ops o);
    void set_opcode(int o)              { set_opcode((if_ops)o); }

    /* comment field operations */
    boolean has_comment() { return (peek_annote(k_comment) != NULL); }
    void append_comment(immed_list *);
    void delete_comment() { get_annote(k_comment); }

    /* clone helper methods */
    virtual instruction *clone_helper(replacements *r, 
                                        boolean no_copy = FALSE)=0; 
    virtual void find_exposed_refs(base_symtab *dst_scope, replacements *r)=0; 

    /* print methods */
    virtual void print(FILE *o_fd=stdout);
    void print_comment(FILE *o_fd, char *comment_char);
};

DECLARE_DLIST_CLASS(milist, machine_instr *);
extern void Print_milist(FILE *, milist *);

/* Label instructions. */
class mi_lab: public machine_instr {
    label_sym   *lab;

  public:
    mi_lab():machine_instr(k_null_string,io_lab) { lab = NULL; }
    mi_lab(mi_ops o, label_sym *s);
    mi_lab(mi_ops o, in_lab *i);

    inst_format format()                { return (inst_format)mif_lab; }

    label_sym *label()                  { return lab; }
    void set_label(label_sym *l)        { lab = l; }

    unsigned num_dsts()                 { return 0; }
    operand dst_op(unsigned n=0)        { return operand(); }
    void set_num_dsts(unsigned)         { return; }     /* dummy method */
    void set_dst(operand)               { no_dst_error(); }
    void set_dst(unsigned, operand)     { no_dst_error(); }

    instruction *clone_helper(replacements *r, boolean no_copy = FALSE);
    void find_exposed_refs(base_symtab *dst_scope, replacements *r); 

    void print(FILE *o_fd=stdout);
};

/* Class for almost all operations.  Called 'rr' for historical reasons. */
class mi_rr: public machine_instr {

  public:
    mi_rr();
    /* constructor for instructions that do NOT write memory */
    mi_rr(mi_ops o,
          operand d = operand(),        /* destination */
          operand s1 = operand(),       /* source 1 */
          operand s2 = operand());      /* source 2 */
    /* constructor for instructions that DO write memory */
    mi_rr(mi_ops o,
          instruction *ea,              /* store effective address */
          operand s1 = operand(),       /* source 1 */
          operand s2 = operand());      /* source 2 */

    virtual inst_format format()        { return (inst_format)mif_rr; }

    /* by convention, we keep the store EA, if it exists, in srcs[0] */
    operand store_addr_op(unsigned n);
    void set_store_addr_op(unsigned n, operand r);
    void remove_store_addr_op(unsigned n);

    virtual instruction *clone_helper(replacements *r, boolean no_copy=FALSE);
    virtual void find_exposed_refs(base_symtab *dst_scope, replacements *r); 

    virtual void print(FILE *o_fd=stdout);
};

/* Branch/jump class.  Instructions in this class may also perform
 * alu operations and/or read/write memory.  */
class mi_bj: public mi_rr {
    sym_node    *targ;          /* may be NULL */

  public:
    mi_bj();
    mi_bj(mi_ops o,
          sym_node *t,                  /* target symbol */
          operand d = operand(),        /* destination */
          operand s1 = operand(),       /* src1 or tgt-reg */
          operand s2 = operand());      /* source 2 */
    mi_bj(mi_ops o,
          instruction *ea,              /* store ea */
          sym_node *t,                  /* target symbol */
          operand d = operand(),        /* destination */
          operand s1 = operand(),       /* src1 or tgt-reg */
          operand s2 = operand());      /* source 2 */

    inst_format format()                { return (inst_format)mif_bj; }

    sym_node *target()                  { return targ; }
    void set_target(sym_node *t)        { targ = t; }
    boolean is_indirect()               { return (targ == NULL); }

    instruction *clone_helper(replacements *r, boolean no_copy = FALSE);
    void find_exposed_refs(base_symtab *dst_scope, replacements *r); 

    void print(FILE *o_fd=stdout);
};

/* Pseudo-op class. */
class mi_xx: public machine_instr {
    immed_list  il;                     /* list of pseudo-op operands */

  public:
    mi_xx():machine_instr(k_null_string, io_null) {}
    mi_xx(mi_ops o);
    mi_xx(mi_ops o, immed i);

    inst_format format()                { return (inst_format)mif_xx; }

    unsigned num_dsts()                 { return 0; }
    operand dst_op(unsigned n=0)        { return operand(); }
    void set_num_dsts(unsigned)         { return; }     /* dummy method */
    void set_dst(operand)               { no_dst_error(); }
    void set_dst(unsigned, operand)     { no_dst_error(); }

    /* pseudo-op instruction operations */
    boolean has_operands()              { return !il.is_empty(); }
    void append_operand(immed i)        { il.append(i); }
    immed pop_operand()                 { return il.pop(); }

    instruction *clone_helper(replacements *r, boolean no_copy = FALSE);
    void find_exposed_refs(base_symtab *dst_scope, replacements *r); 

    void print(FILE *o_fd=stdout);
};

#endif
