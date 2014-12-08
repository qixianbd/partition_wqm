/* Interface of class mapping instructions to definition points */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef DEF_CATALOG_H
#define DEF_CATALOG_H

/*
 * Class def_catalog
 *
 * Map between an instruction and its "definition point identifiers".
 *
 * A definition point is a point in the program at which some quantity
 * (typically a variable) is defined.  Since an instruction may define
 * zero, one, or many such things, it may embody multiple definition
 * points.
 *
 * Class `def_catalog' can be used to number the definition points in a
 * procedure being compiled, assigning a sequence of consecutive small
 * integers to each definition instruction.  Such integer identifiers can
 * then be used to index bit vectors in data flow analysis.
 *
 * If `instr' is an instruction pointer, `index' is a definition
 * point identifier, and `count' is the number of definition points
 * represented by `instr', then:
 *
 * num_defs()		returns the number of definition points in the catalog
 *			so far.
 * enter(instr, count)	enters `instr' in the catalog, associated with a
 *			sequence of `count' definition point id's.  Returns 
 *			the first definition point id of the sequence, or
 *			else -1 if `count' is 0.
 * lookup(index)	returns (a pointer to) the instruction containing
 *			the definition point whose id is `index'.  Returns
 *			NULL if no such instruction in in the catalog.
 * first_id(instr)	returns the first definition point id for `instr',
 *			or else -1 if `instr' has no definition points or
 *			isn't cataloged.
 * num_ids(instr)	returns the number of definition point id's for
 *			`instr', or else -1 if `instr' has no definition
 *			points or isn't cataloged.
 */

DECLARE_X_ARRAY(instruction_array, instruction, 1000);

class def_catalog {
public:
    def_catalog();
    ~def_catalog();

    int num_defs();
    int enter(instruction *instr, int count);
    instruction *lookup(int index);
    int first_id(instruction *instr);
    int num_ids(instruction *instr);

private:
    int number;
    instruction_array *table;
};


/*
 * Annotations mapping an instruction to:
 *   the id of its first definition point, and
 *   the number of its definition points,
 * respectively.
 */

extern char *k_def_point_1st;
extern char *k_def_point_num;

#endif /* DEF_CATALOG_H */
