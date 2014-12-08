/* bitset_function.cc implements functions 
 *documented in bitset_function.{h/tex} 
 */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>
#pragma implementation "bitset_function.h"
				// pick up header files 
#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

bitset_function::bitset_function(int first_unused_bit_number) {
  hbn = first_unused_bit_number;// store bit numbers in bit_set compatible form
  id = new bit_set(0,hbn);	// set up identity boolean function
  id->universal();
  cs = new bit_set(0,hbn);
  cs->clear();
}

bitset_function::bitset_function() {
  hbn = 0;
  id = NULL;
  cs = NULL;
}

bitset_function::~bitset_function() {
  delete id;
  delete cs;
}

bitset_function::bitset_function(const bitset_function& i) { 
  // initialize this from i.
  hbn = i.hbn;
  id = new bit_set(0,hbn);
  cs = new bit_set(0,hbn);
  *id = *(i.id);
  *cs = *(i.cs);
}

bitset_function& bitset_function::operator=(const bitset_function& i) {
  if (this != &i) {
    delete id;
    delete cs;
    hbn = i.hbn;
    id = new bit_set(0,hbn);
    cs = new bit_set(0,hbn);
    *id = *(i.id);
    *cs = *(i.cs);
  }
  return *this;
}

/* The following are now inlined
void bitset_function::topfn() {
  id->clear();			// set function to "constant 1" on all bits
  cs->universal();
}

void bitset_function::topfn(int bitnum) {
  assert_msg(bitnum >= 0 && bitnum < hbn,
    ("bitset_function::topfn - bit %d not in range %d-%d", bitnum,0,hbn));
  id->remove(bitnum);
  cs->add(bitnum);
}

void bitset_function::botfn() {
  id->clear();			// set function to "constant 0" on all bits
  cs->clear();
}

void bitset_function::botfn(int bitnum) {
  assert_msg(bitnum >= 0 && bitnum < hbn,
     ("bitset_function::botfn - bit %d not in range %d-%d", bitnum,0,hbn));
  id->remove(bitnum);
  cs->remove(bitnum);
}

void bitset_function::idfn() {
  id->universal();		// set function to "identity" on all bits
  cs->clear();
}

void bitset_function::idfn(int bitnum) {
  assert_msg(bitnum >= 0 && bitnum < hbn,
     ("bitset_function::idfn - bit %d not in range %d-%d", bitnum,0,hbn));
  id->add(bitnum);
  cs->remove(bitnum);
}


void bitset_function::apply(bit_set* v) {
  *v *= *id;			// if identity function, keep bits
  *v += *cs;			// set bits by const fns
				// requires (cs == cs - id)
}

void bitset_function::compose(const bitset_function* g) {
  bit_set tmp(0,hbn);		// temporary bit set in scope
  tmp = *id;
  tmp *= *(g->cs);
  *cs += tmp;
  *id *= *(g->id);
}
* end of code moved inline 
*/

void bitset_function::print(FILE* fp) {
    int i;
    fprintf(fp, "(%d:%d){", 0, hbn);
    for (i = 0; i < hbn; i++) {
      if (!id->contains(i))
	fprintf(fp, "%s", (cs->contains(i) ? "1" : "0"));
      else
	fprintf(fp, "%s", (cs->contains(i) ? "x" : "i"));
    }
    putc('}', fp);
}
