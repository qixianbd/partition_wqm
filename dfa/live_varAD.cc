/* Live-variable analysis implementation */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"
#include "bit_vector_dfa_plus.h"
#include "live_varAD.h"
/* 
 *  Constructing an instance of the class invokes liveness analysis.
 */

live_varad_problem::live_varad_problem(cfg *graph, operand_bit_manager *obm,
				       instruction_def_use_f du_fun) :
  bit_vector_problem_plus(backward, standard_t, union_p, in_k, obm->num_bits(),
			  graph)
{
  
  bit_man = obm;
  instr_def_use_fun = du_fun;
  latest = NULL;
  instr_def = new bit_set(0, bitset_size); // super::bitset_size
  instr_use = new bit_set(0,bitset_size); // super::bitset_size

  solve();			// super::solve()
}

live_varad_problem::~live_varad_problem() {
  delete instr_def;
  delete instr_use;
  /* ~super() */
}

boolean
live_varad_problem::live_out(cfg_node *n, int var_num)
{
    return in_set_contains(n,var_num);
}

bit_set *
live_varad_problem::live_out_set(cfg_node *n)
{
    return in_set(n);
}

/* Iterators to produce (presumably sparse) gen and kill sets 
 * for an instruction.
 *
 * Very silly to be converting a bit set to an iteration which is internally 
 * reconverted to a bit set, but this is just a feasability test...
 */
int live_varad_problem::defs() {
  int i;
  for (i = defs_place; i < instr_def->ub(); i++) {
    if (instr_def->contains(i)) {
      defs_place = i+1;
      return i;
    }
  }
  return -1;
}

int live_varad_problem::kill_iterator() {
  return defs();
}

int live_varad_problem::uses() {
  int i;
  for (i = uses_place; i < instr_use->ub(); i++) {
    if (instr_use->contains(i)) {
      uses_place = i+1;
      return i;
    }
  }
  return -1;
}

int live_varad_problem::gen_iterator() {
  return uses();
}


void live_varad_problem::defs_uses_init(instruction* in) {
    if (in != latest) {
      instr_def->clear();
      instr_use->clear();
      instr_def_use_fun(in, bit_man, instr_def, instr_use);
      latest = in;
    }
    defs_place = 0;
    uses_place = 0;
}

void live_varad_problem::gen_iterator_init(instruction* i) {
  defs_uses_init(i);
}

void live_varad_problem::kill_iterator_init(instruction* i) {
  defs_uses_init(i);
}
