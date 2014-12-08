/* Bit-vector-based dataflow analysis problems */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef BIT_VECTOR_DFA_H
#define BIT_VECTOR_DFA_H

const int INFINITY = 0x7fffffff;
enum DFA_direction_type { forward, backward };

class bit_vector_problem {
  private:
    DFA_direction_type dfa_dir;

  protected:
    cfg *the_cfg;
    DFA_direction_type direction()    { return dfa_dir; }

    virtual void build_sets() = 0;
    virtual boolean solver_ops(cfg_node *n) = 0;

  public:
    bit_vector_problem(DFA_direction_type dir, cfg *graph);
    boolean solve(int iteration_limit = INFINITY);
};

#endif /* BIT_VECTOR_DFA_H */
