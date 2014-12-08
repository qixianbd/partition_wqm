/* Implementation of class mapping mov instructions to small integers */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

char *k_instr_catalog;

instr_catalog::instr_catalog(cfg* graph)
{
  number = 0;
  table = new instr_array;
}  

instr_catalog::~instr_catalog()
{
  immed_list *iml;
  for (int i = 0; i < num_defs(); i++) {
    iml = (immed_list*)(*table)[i]->mi->get_annote(k_instr_catalog);
    if (iml) delete(iml);
    }
    delete table;
}

int instr_catalog::num_defs()
{
    return number;
}

int instr_catalog::enroll(machine_instr* instr)
{
    int inst_id = index(instr);
    if (inst_id >= 0) {
      return inst_id;
    } else {
      immed_list *il = new immed_list();
      array_typ* element = new array_typ;

      il->append(immed(number));
      instr->append_annote(k_instr_catalog, il);
      element->mi = instr;

      assert_msg((number == table->ub()), 
		 ("instr_catalog::enroll out of synch"));
      table->extend(element);

      int prev_number = number;
      number++;
      return prev_number;
    }
}

machine_instr* instr_catalog::instr(int index)
{
  assert_msg(((unsigned)index < (unsigned)num_defs()),
	     ("instr_catalog::instr range error"));
  return (*table)[index]->mi;
}

int instr_catalog::index(machine_instr* instr)
{
  immed_list* iml;
  iml = (immed_list*)instr->peek_annote(k_instr_catalog);
  if (!iml) return -1;		// no index for this instruction
  return (*iml)[0].integer();	// integer index
}
