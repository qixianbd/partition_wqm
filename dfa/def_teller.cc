/* Implementation of a class to enumerate the "definees" of an instruction */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"


static void
Def_operand(operand, operand_bit_manager *, definee_list *);
static void
Def_on_entry(proc_sym *, operand_bit_manager *, definee_list *);
static void
Def_by_call(instruction *, operand_bit_manager *, definee_list *);
static void
Def_whole_convention(operand_bit_manager *, int, int, definee_list *);


definee_list *
var_def_teller::definees(instruction *instr)
{
    definee_list *result = new definee_list;

    for (unsigned i = 0; i < instr->num_dsts(); i++)
        Def_operand(instr->dst_op(i), the_obm, result);

    if (instr->peek_annote(k_proc_entry))
        Def_on_entry(the_psym, the_obm, result);

    return result;
}

int
var_def_teller::num_definees()
{
    return the_obm->num_bits();
}


definee_list *
reg_def_teller::definees(instruction *instr)
{
    definee_list *result = new definee_list;

    for (unsigned i = 0; i < instr->num_dsts(); i++)
        Def_operand(instr->dst_op(i), the_obm, result);

    if (Is_call(instr))
        Def_by_call(instr, the_obm, result);

    else if (instr->peek_annote(k_proc_entry))
        Def_on_entry(the_psym, the_obm, result);

    return result;
}

int
reg_def_teller::num_definees()
{
    return the_obm->num_bits();
}


/* Def_operand:  Append an operand's definee index to the list of definees,
 * provided the operand is acceptable to the operand bit manager.
 */

static void
Def_operand(operand opnd, operand_bit_manager *bit_man, definee_list *definees)
{
    int index = -1, count;
    bit_man->enroll(opnd, &index, &count);
    if (index >= 0)
        do 
            definees->append(index++);
        while (--count);
}


/* Def_on_entry: At a procedure entry point, make implicit defini-
 * tions for any parameter registers used, and also for the symbols of
 * parameters that are passed on the stack, since generated code
 * doesn't set them exlicitly, as it does for register-passed
 * parameters.  */

static void
Def_on_entry(proc_sym *the_psym, operand_bit_manager *bit_man,
             definee_list *definees)
{
    sym_node_list_iter pi(the_psym->block()->proc_syms()->params());

    while (!pi.is_empty()) {
        var_sym *param = (var_sym *)pi.step();
        operand opnd;

        if (param->peek_annote(k_vsym_info) && vsym_passed_in_preg(param))
            opnd = operand(vsym_get_preg(param), param->type());
        else
	    opnd = operand(param);          

        Def_operand(opnd, bit_man, definees);
    }
}



/* Def_by_call: Augment the definees for a call instruction to reflect all
 * the registers that the callee may define or destroy.
 *
 * FIXME: merge with similar utility in live_var.cc
 */

static void
Def_by_call(instruction *instr, operand_bit_manager *bit_man,
            definee_list *definees)
{
    for (int b = 0; b < LAST_REG_BANK; b++) {
	Def_whole_convention(bit_man, b, ARG, definees);
	Def_whole_convention(bit_man, b, RET, definees);
	Def_whole_convention(bit_man, b, TMP, definees);
	Def_whole_convention(bit_man, b, ASM_TMP, definees);
    }
}

/* Def_whole_convention: Apply Def_operand to all the registers in a given
 * bank and convention.  (A register operand with type void is treated as if
 * the type had the natural register width.)
 *
 * FIXME: merge with similar utility in live_var.cc
 */

static void
Def_whole_convention(operand_bit_manager *bit_man, int bank, int conv,
		     definee_list *definees)
{
    int n = target_regs->num_in(bank, conv);

    if (!n)
	return;

    int rr = REG(bank, conv, 0);

    do {
        Def_operand(operand(rr++, type_void), bit_man, definees);
    } while (--n);
}
