/* file "eaHelper.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef EAHELPER_H
#define EAHELPER_H

/* Routines that help to create EA operands */
extern instruction *New_ea_base_p_disp(operand b, long d);
extern instruction *New_ea_base_p_disp(operand b, immed di);

extern instruction *New_ea_symaddr(sym_node *s, unsigned d = 0);
extern instruction *New_ea_symaddr(immed si);

extern instruction *New_ea_indexed_symaddr(operand i, sym_node *s, long d);

extern instruction *New_ea_base_p_indexS_p_disp(operand b, operand i,
                                                unsigned s, long d);

/* Routines that answer common queries about an EA expression tree */
extern boolean Is_ea_base_p_disp(instruction *);
extern boolean Is_ea_symaddr(instruction *);
extern boolean Is_ea_indexed_symaddr(instruction *);
extern boolean Is_ea_indexS_p_disp(instruction *);
extern boolean Is_ea_base_p_index_p_disp(instruction *);
extern boolean Is_ea_base_p_indexS_p_disp(instruction *);

/* Routines that help access parts of an EA calculation */
extern sym_node *Get_ea_symaddr_sym(instruction *);
extern int Get_ea_symaddr_off(instruction *);

// Unfinished code not yet needed.
// typedef void (*ea_map_f)(instruction *ea, void *x);
// extern void Map_ea(instruction *ea, ea_map_f f, void *x);

#endif /* EAHELPER_H */
