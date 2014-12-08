/* Interface of class mapping instructions to definition points */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>
#ifndef INSTR_CATALOG_H
#define INSTR_CATALOG_H
#include <machine.h>

/*
 * Class `instr_catalog'
 *
 * Map between instructions and consecutive small integers suitable
 * to index bit vectors or other dense structures in flow analysis.
 *
 * If `instr' is an instruction pointer, `index' is the small integer
 * associated with a instr:
 *
 * num_defs()		returns the number of instrs in the catalog so far.
 * lookup(index)	returns (a pointer to) the instr `instr'
 *			associated with `index'.  Returns NULL if no such 
 *                      instruction in in the catalog.
 *
 * more...
 */

class instr_catalog {
protected:
  typedef struct {machine_instr* mi; } array_typ;
DECLARE_X_ARRAY(instr_array, array_typ, 1000);

    int number;
    instr_array* table;		// extensible array of array_typ

public:
    instr_catalog(cfg* graph);
    ~instr_catalog();

    int enroll(machine_instr* instr); /* returns index */
    int num_defs();		/* number of (distinct) enrollees so far */
    machine_instr* instr(int index);
    int index(machine_instr*);
};


/*
 * Annotations mapping an instruction to its instr_catalog number
 */

extern char *k_instr_catalog;

#endif /* INSTR_CATALOG_H */
