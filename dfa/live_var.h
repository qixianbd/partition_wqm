/* Interface for bit-vector-based live-variable analyzer */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef LIVE_VAR_H
#define LIVE_VAR_H

DECLARE_X_ARRAY(bit_set_array, bit_set, 1024);

void Delete_bit_set_array(bit_set_array *);

void Instruction_def_use(instruction *, operand_bit_manager *,
                         bit_set *def, bit_set *use);

class live_var_problem : protected bit_vector_problem {
  protected:
    typedef void (*instruction_def_use_f)
                 (instruction *, operand_bit_manager *, bit_set *, bit_set *);

  public:
    live_var_problem(cfg *, operand_bit_manager *,
                     instruction_def_use_f = Instruction_def_use);
    virtual ~live_var_problem();

    boolean live_in(cfg_node *n, int var_num);
    boolean live_out(cfg_node *n, int var_num);
    bit_set *live_in_set(cfg_node *n);
    bit_set *live_out_set(cfg_node *n);

  protected:
    operand_bit_manager *bit_man;
    bit_set_array *defs;
    bit_set_array *uses;
    bit_set_array *live_ins;
    bit_set_array *live_outs;

    void build_sets();
    boolean solver_ops(cfg_node *);

    virtual void build_local_info(int n);
    virtual void note_instr_def_use(int node, instruction *,
                                    bit_set *def, bit_set *use);

  protected:
    instruction_def_use_f instr_def_use_fun;
    bit_set *instr_def;
    bit_set *instr_use;
};
#endif /* LIVE_VAR_H */
