/* Highly parameterized bit-vector-based dataflow analysis problems */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef BIT_VECTOR_DFA_PLUS_H
#define BIT_VECTOR_DFA_PLUS_H
/* Using an enumeration and constant defined in bit_vector_dfa.h */
#include "bit_vector_dfa.h"

DECLARE_X_ARRAY(DFAP_bit_set_array, bit_set, 100);
DECLARE_X_ARRAY(DFAP_bitset_function_array, bitset_function, 100);
  /* enum DFA_direction_type { forward, backward }; 
     // defined in bit_vector_dfa.h */
  enum DFA_transfer { standard_t, special_t };
  enum DFA_propagation { isect_p, union_p, special_p };
  enum DFA_keep_info { in_k, out_k, both_k };
  /* const int INFINITY = 0x7fffffff; // defined in bit_vector_dfa.h */

class bit_vector_problem_plus {
private:
  DFAP_bitset_function_array* bb_xfer;    // transfer function: per block
  DFAP_bit_set_array* bb_in;      // in bitset: per block
  DFAP_bit_set_array* bb_out;     // out bitset: per block

  void meet(cfg_node*);           // code pulled out of solve for readability
  void build_local_info(int);     // code pulled out of solve for readability

protected:
    int bitset_size;
    virtual void gen_iterator_init(instruction*) = 0;
    virtual int gen_iterator() = 0;
    virtual void kill_iterator_init(instruction*) = 0;
    virtual int kill_iterator() = 0;
    cfg* the_cfg;
    bitset_function* entry_xfer_fn;
    bitset_function* exit_xfer_fn;
    virtual bit_set* special_propagation(cfg_node_list_iter*);
    virtual void special_transfer(bitset_function*);
    
    DFA_direction_type direction;
    DFA_propagation propagation;
    DFA_transfer transfer;
    DFA_keep_info needed_later;

    void Delete_DFAP_bit_set_array(DFAP_bit_set_array*);
    void Delete_DFAP_bitset_function_array(DFAP_bitset_function_array*);

public:
  bit_vector_problem_plus(
                          DFA_direction_type dir, 
                          DFA_transfer at_each_instr,
                          DFA_propagation propagation,
                          DFA_keep_info needed_later,
                          int bitset_size,
                          cfg* the_cfg,

                          bitset_function* entry_xfer_fn = NULL,
                          bitset_function* exit_xfer_fn = NULL
                          );
  virtual ~bit_vector_problem_plus();

  bit_set* in_set(cfg_node* basic_block);
  boolean in_set_contains(cfg_node* basic_block, int bit_num);
  bit_set* out_set(cfg_node* basic_block);
  boolean out_set_contains(cfg_node* basic_block, int bit_num);

  boolean solve(int iteration_limit = INFINITY);
  virtual void update_bit_set_size(int);
  void update_bit_set_for_instr(bit_set*,instruction*);
  void print(cfg_node* bb, FILE *fp = stdout); // debugging only
};

#endif /* BIT_VECTOR_DFA_PLUS_H */
