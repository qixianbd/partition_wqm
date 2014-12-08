/* Interface for bit-vector-based unset variable analyzer */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef UNSET_VAR_H
#define UNSET_VAR_H

class unset_var_problem : protected bit_vector_problem {
    typedef void (*instruction_def_use_f)
                 (instruction *, operand_bit_manager *, bit_set *, bit_set *);

  public:
    unset_var_problem(cfg *, operand_bit_manager *,
                      instruction_def_use_f = Instruction_def_use);
    virtual ~unset_var_problem();

    boolean unset_in(cfg_node *n, int var_num);
    boolean unset_out(cfg_node *n, int var_num);
    bit_set *unset_in_set(cfg_node *n);
    bit_set *unset_out_set(cfg_node *n);

  protected:
    operand_bit_manager *bit_man;
    bit_set_array *defs;
    bit_set_array *unset_ins;
    bit_set_array *unset_outs;

    void build_sets();
    boolean solver_ops(cfg_node *);

    virtual void build_local_info(int n);
    virtual void note_instr_def(int node, instruction *, bit_set *def);

  private:
    instruction_def_use_f instr_def_use_fun;
    bit_set *instr_def;
    bit_set *instr_use;
};
#endif /* UNSET_VAR_H */
