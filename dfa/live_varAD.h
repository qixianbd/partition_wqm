/* Interface for bit-vector-based live-variable analyzer */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef LIVE_VARAD_H
#define LIVE_VARAD_H

void Instruction_def_use(instruction *, operand_bit_manager *,
			 bit_set *def, bit_set *use);

class live_varad_problem : protected bit_vector_problem_plus {
  typedef void (*instruction_def_use_f)(instruction *, 
					operand_bit_manager *, 
					bit_set *, bit_set *);

  private:
    void defs_uses_init(instruction*); /* use == gen, def == kill */
    int uses();
    int defs();
    instruction_def_use_f instr_def_use_fun;
    instruction* latest;	/* shared between uses_init, defs_init */
    bit_set *instr_def;		/* shared between ..._init, defs, uses */
    bit_set *instr_use;		/* shared between ..._init, defs, uses */
    int defs_place;		/* shared between calls to defs */
    int uses_place;

  protected:
    /* our bit manager */
    operand_bit_manager *bit_man;

    /* Override methods in superclass */
    void gen_iterator_init(instruction*);
    int gen_iterator();
    void kill_iterator_init(instruction*);
    int kill_iterator();
    
  public:
    live_varad_problem(cfg *, operand_bit_manager *,
                     instruction_def_use_f = Instruction_def_use);
    ~live_varad_problem();

    boolean live_out(cfg_node *n, int var_num);
    bit_set *live_out_set(cfg_node *n);
    void print(cfg_node* bb, FILE *fp = stdout) {
      bit_vector_problem_plus::print(bb,fp);
    }
};

#endif /* LIVE_VARAD_H */
