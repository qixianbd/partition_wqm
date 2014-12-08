/* bit_vector_problem_plus.cc implements functions documented in 
 * bit_vector_problem_plus.{h/tex} */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#pragma implementation "bit_vector_problem_plus.h"

/* pick up header files */
#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

/*
 * Initialize relevant fields in  
 */
bit_vector_problem_plus ::
bit_vector_problem_plus(
			  DFA_direction_type dir, 
			  DFA_transfer at_each_instr,
			  DFA_propagation propagation,
			  DFA_keep_info needed_later,
			  int bitset_size,
			  cfg *the_cfg,
			  
			  bitset_function* entry_xfer_fn,
			  bitset_function* exit_xfer_fn
			  ) {
  /* Copy parameters to local storage */
  direction = dir;
  transfer = at_each_instr;
  this->propagation = propagation;
  this->needed_later = needed_later;
  this->bitset_size = bitset_size;
  this->the_cfg = the_cfg;
  this->entry_xfer_fn = entry_xfer_fn;
  this->exit_xfer_fn = exit_xfer_fn;
  bb_xfer = NULL;
  bb_in = bb_out = NULL;
}

/*
 * Destructor: clean up any data that the user asked to have kept during 
 * analysis.
 */
bit_vector_problem_plus ::
~bit_vector_problem_plus() {
  /* Delete any bit sets that weren't cleaned up after solve */
  if (bb_in && needed_later != out_k) Delete_DFAP_bit_set_array(bb_in);
  if (bb_out && needed_later != in_k) Delete_DFAP_bit_set_array(bb_out);
}

/*
 * Handle multiple successors / predecessors as needed for
 * forward / backward analysis with union / intersection / special
 * as meet operator.
 */
void bit_vector_problem_plus ::
meet(cfg_node* n) {
  int nnum = n->number();
  cfg_node_list* outs = (direction == forward) ? n->preds() : n->succs();
  cfg_node_list_iter s_iter(outs);

  bit_set* in = (*bb_in)[nnum];
  switch (propagation) {

  case isect_p:
    in->universal();
    while (!s_iter.is_empty()) {
      *in *= *((*bb_out)[s_iter.step()->number()]);
    }
    break;

  case union_p:
    in->clear();
    while (!s_iter.is_empty()) {
      *in += *((*bb_out)[s_iter.step()->number()]);
    }
    break;

  case special_p:
    *in = *special_propagation(&s_iter);
    break;
  }
}

/*
 * Build bb_xfer for a basic block.
 * Handle forward / backward, special initialization for entry and exit,
 * and special transfer function.
 */
void  bit_vector_problem_plus ::
build_local_info(int i) {

  cfg_node* bb = the_cfg->node(i);

  /* set up the in set and xfer function for entry or exit blocks. */
  if (bb->is_begin()) {
    if (propagation == isect_p) {
      (*bb_in)[i]->universal();	// if intersecting override in with universal
    }
    if (entry_xfer_fn != NULL) *((*bb_xfer)[i]) = *entry_xfer_fn;
  } else if (bb->is_end()) {
    if (propagation == isect_p) {
      (*bb_in)[i]->universal();	// if intersecting override in with universal
    }
    if (exit_xfer_fn != NULL) *((*bb_xfer)[i]) = *exit_xfer_fn;
  } 
  
  /* ignore label blocks */
  else if (bb->is_label()) { 
    return;			// no relevant information for labels
  } 

  /* set up transfer function for standard blocks */ 
  else if (bb->is_instr() || bb->is_block()) {
    // f is automatically initialized to identity function
    bitset_function f(bitset_size);
    cfg_node_instr_iter inst_iter(bb, (direction == backward));
    instruction* inst = NULL;
    int bitnum;
    
    switch (transfer) {
    case standard_t:
      
      /* iterate over instrucions in block */
      while (!inst_iter.is_empty()) {
	inst = inst_iter.step()->instr();
	
	kill_iterator_init(inst);
	while (0 <= (bitnum = kill_iterator())) {
	  assert(bitnum < bitset_size); // never trust derived code...
	  f.botfn(bitnum);	// remove kills
	}
	
	gen_iterator_init(inst);
	while (0 <= (bitnum = gen_iterator())) {
	  assert(bitnum < bitset_size);
	  f.topfn(bitnum);	// add gens
	}
      }
      break;
      
    case special_t:
      
      special_transfer(&f);
      break;
    }

    *((*bb_xfer)[i]) = f;
  }

  /* unknown node type */
  else {
    assert_msg(FALSE, 
	       ("bit_vector_dfa_plus: can't deal with cfg block of kind ",
		bb->kind()));
  }
}


boolean bit_vector_problem_plus ::
solve(int iteration_limit) {
  int iterations = 0;

  bb_xfer = new DFAP_bitset_function_array();
  bb_in = new DFAP_bit_set_array();
  bb_out = new DFAP_bit_set_array();

  /* build local bb information: bb_xfer, and allocate other bit_sets */
  for (unsigned i = 0; i < the_cfg->num_nodes(); i++) {
    bb_xfer->extend(new bitset_function(bitset_size)); // identity by default
    bb_in->extend(new bit_set(0, bitset_size)); // clear by default
    bb_out->extend(new bit_set(0, bitset_size)); // clear by default
    build_local_info(i);
  }

  /* Initialize to solve bit vector equations by iteration. */
  boolean changes = TRUE;

  /* initialize traversal list in depth-first order for forward
   * problems, and reverse dfo for backward ones (dragon
   * book [ASU], section 10.10) */
  boolean fwd = (direction == forward);
  cfg_node_list *df_list = the_cfg->reverse_postorder_list(fwd);
  
  /* iterate till fixed-point or till exceed iteration_limit */

  /* Future work:
   * Use work list here rather than looping over all bbs all the time! 
   * we would have to be careful not to double insert a bb, but we could 
   * avoid repeatedly processing bbs whose IN() sets haven't changed.
   */

  while (changes && (iterations < iteration_limit)) {
    iterations++;
    changes = FALSE;
    
    cfg_node_list_iter df_list_iter(df_list);
    while (!df_list_iter.is_empty()) {
      cfg_node *n = df_list_iter.step(); 
      int nnum = n->number();
      bit_set new_out(0, bitset_size);

      meet(n);			// create new "in" value from predecessors
      new_out = *((*bb_in)[nnum]); // store "in" value
      ((*bb_xfer)[nnum])->apply(&new_out); // make it new out value
      
      if (*((*bb_out)[nnum]) != new_out) {
	*((*bb_out)[nnum]) = new_out; // changed: copy new_out to out
	changes = TRUE;		// and record change
      }
    }
  }
  
  /* clean up and return */
  Delete_DFAP_bitset_function_array(bb_xfer);
  if (needed_later == in_k) Delete_DFAP_bit_set_array(bb_out);
  if (needed_later == out_k) Delete_DFAP_bit_set_array(bb_in);
  return (iterations < iteration_limit);
}

/*
 * Public methods for testing the results of a bit vector analysis.
 */

bit_set* bit_vector_problem_plus ::
in_set(cfg_node* bb) {
  assert(needed_later != out_k); // check that we kept bb_in
  bit_set* internal_bs = (*bb_in)[bb->number()]; // get right pointer
  return internal_bs;		// give caller access to our guts
}

boolean bit_vector_problem_plus ::
in_set_contains(cfg_node* bb, int bit_num) {
  assert(needed_later != out_k); // check that we kept bb_in
  assert(bit_num < bitset_size);
  bit_set* internal_bs = (*bb_in)[bb->number()]; // get right pointer
  return internal_bs->contains(bit_num);
}

bit_set* bit_vector_problem_plus ::
out_set(cfg_node* bb) {
  assert(needed_later != in_k);	// check that we kept bb_out
  bit_set* internal_bs = (*bb_out)[bb->number()]; // get right pointer
  return internal_bs;		// give caller access to our guts
}

boolean bit_vector_problem_plus ::
out_set_contains(cfg_node* bb, int bit_num) {
  assert(needed_later != in_k); // check that we kept bb_out
  assert(bit_num < bitset_size);
  bit_set* internal_bs = (*bb_out)[bb->number()]; // get right pointer
  return internal_bs->contains(bit_num);
}

void bit_vector_problem_plus ::
update_bit_set_size(int bs) 
{
  bitset_size = bs;
}

/*
 * Public method for processing an instruction just involves using
 * the derived classes iterators to update gen ans kill information.
 */
void bit_vector_problem_plus ::
update_bit_set_for_instr(bit_set* bs,instruction* inst) {
  int bitnum;
  
  kill_iterator_init(inst);
  while (0 <= (bitnum = kill_iterator())) {
    assert(bitnum < bitset_size); // never trust derived code...
    bs->remove(bitnum);
  }
  
  gen_iterator_init(inst);
  while (0 <= (bitnum = gen_iterator())) {
    assert(bitnum < bitset_size);
    bs->add(bitnum);
  }
}

/*
 * Some cleanup code
 */
void bit_vector_problem_plus ::
Delete_DFAP_bit_set_array(DFAP_bit_set_array *a)
{
  if (a) {
    for (int i = 0; i < a->ub(); i++) delete (*a)[i];
    delete a;
  }
}

void bit_vector_problem_plus ::
Delete_DFAP_bitset_function_array(DFAP_bitset_function_array *a)
{
  if (a) {
    for (int i = 0; i < a->ub(); i++) delete (*a)[i];
    delete a;
  }
}

/*
 * The following virtual methods handle special cases.
 * They have code that creates run time error messages, rather than being 
 * pure virtual, so that derived classes don't have to create code for them.
 */
bit_set* bit_vector_problem_plus ::
special_propagation(cfg_node_list_iter* it) {
  assert_msg(FALSE, ("bit_vector_problem_plus::special_propagation must be overridden"));
  return (bit_set*)0;
}

void bit_vector_problem_plus ::
special_transfer(bitset_function* f){
  assert_msg(FALSE, ("bit_vector_problem_plus::special_transfer must be overridden"));
}

void bit_vector_problem_plus::print(cfg_node* bb, FILE *fp) {
  int i = bb->number();
  
  if (needed_later != out_k) {
    fprintf(fp, "in:   ");
    if ((*bb_in)[i]) (*bb_in)[i]->print(fp,"%d,");
  }
  if (needed_later != in_k) {
    fprintf(fp, "out:  ");
    if ((*bb_out)[i]) (*bb_out)[i]->print(fp,"%d,");
  }
}
