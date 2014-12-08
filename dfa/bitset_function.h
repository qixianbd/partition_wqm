/*  Bit Set Definitions */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef BITSET_FUNCTION_H
#define BITSET_FUNCTION_H

#pragma interface

class bitset_function {
private:
  

protected:
  int hbn;
  bit_set* id;
  bit_set* cs;
  bitset_function();

public:
  bitset_function(int initial_number_of_bits);
  ~bitset_function();
  bitset_function(const bitset_function&);
  bitset_function& operator=(const bitset_function&);

  void topfn()            {
                            id->clear();                  // set function to "constant 1" on all bits
                            cs->universal();
                          }

  void topfn(int bitnum)  {
                            assert_msg(bitnum >= 0 && bitnum < hbn,
                              ("bitset_function::topfn - bit %d not in range %d-%d", bitnum,0,hbn));
                            id->remove(bitnum);
                            cs->add(bitnum);
                          }

  void botfn()            {
                            id->clear();                  // set function to "constant 0" on all bits
                            cs->clear();
                          }

  void botfn(int bitnum)  {
                            assert_msg(bitnum >= 0 && bitnum < hbn,
                               ("bitset_function::botfn - bit %d not in range %d-%d", bitnum,0,hbn));
                            id->remove(bitnum);
                            cs->remove(bitnum);
                          }

  void idfn()             {
                            id->universal();              // set function to "identity" on all bits
                            cs->clear();
                          }

  void idfn(int bitnum)   {
                            assert_msg(bitnum >= 0 && bitnum < hbn,
                               ("bitset_function::idfn - bit %d not in range %d-%d", bitnum,0,hbn));
                            id->add(bitnum);
                            cs->remove(bitnum);
                          }


  void apply(bit_set* updated) {
                                 *updated *= *id;              // if identity function, keep bits
                                 *updated += *cs;              // set bits by const fns
                                                               // requires representation invariant
                                                               // that (cs == cs - id)
                               }

  void compose(const bitset_function* old) {
                                             bit_set tmp(0,hbn);           // temporary bit set in scope
                                             tmp = *id;
                                             tmp *= *(old->cs);
                                             *cs += tmp;
                                             *id *= *(old->id);
                                           }

  void print(FILE* fp = stdout);
};
#endif /* BITSET_FUNCTION_H */
