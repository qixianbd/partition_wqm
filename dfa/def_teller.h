/* Interface of a class to enumerate the "definees" of an instruction */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef DEF_TELLER_H
#define DEF_TELLER_H

DECLARE_LIST_CLASS(definee_list, int);

/*
 * A `def_teller' maps an instruction to a list of zero or more small
 * integers, each uniquely identifying a quantity that instruction defines.
 *
 * The abstract class doesn't specify what kinds of quantities an
 * instruction may define.  That's left to concrete subclasses.
 *
 * (teller: One who counts or keeps tally.)
 *
 * Methods: if `instr' is an instruction pointer', then:
 *
 * definees(instr)	produces a (pointer to a) definee_list containing
 *			the integer identifiers of all items defined by
 *			`instr'.  (List should not be altered by caller.)
 * num_definees()	returns the number of distinct definees seen by
 *			definees() since this def_teller instance was
 *			constructed.
 */

class def_teller {
public:
    def_teller() { }
    virtual ~def_teller() { }

    virtual definee_list *definees(instruction *instr) = 0;
    virtual int num_definees() = 0;
};


/*
 * A `var_def_teller' identifies two kinds of definees in a machine
 * instruction: virtual registers and local variables whose address is
 * never taken.
 */

class var_def_teller : public def_teller {
public:
    var_def_teller(proc_sym *psym_, operand_bit_manager *obm_ = NULL) {
        the_psym = psym_;
        the_obm = obm_;
        own_obm = (the_obm ? NULL : (the_obm = new operand_bit_manager));
    }
    virtual ~var_def_teller() { delete own_obm; }

    definee_list *definees(instruction *instr);
    int num_definees();

protected:
    proc_sym *the_psym;
    operand_bit_manager *the_obm, *own_obm;
};


/*
 * A `reg_def_teller' identifies register-like definees in a machine
 * instruction: hard registers, virtual registers, and local variable
 * whose address is never taken.
 */

class reg_def_teller : public def_teller {
public:
    reg_def_teller(proc_sym *psym_, operand_bit_manager *obm_ = NULL) {
        the_psym = psym_;
        the_obm = obm_;
        own_obm = (the_obm ? NULL : (the_obm = new operand_bit_manager));
    }
    virtual ~reg_def_teller() { delete own_obm; }

    definee_list *definees(instruction *instr);
    int num_definees();

protected:
    proc_sym *the_psym;
    operand_bit_manager *the_obm, *own_obm;
};

#endif /* DEF_TELLER_H */
