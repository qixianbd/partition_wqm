/* file "annoteHelper.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef ANNOTEHELPER_H
#define ANNOTEHELPER_H

/*              vsym_in_reg(var_sym *);         #* use SUIF is_reg() method */
extern void     vsym_clear_hreg(var_sym *);     /* hard register methods */
extern void     vsym_set_hreg(var_sym *, int);
extern int      vsym_get_hreg(var_sym *);
extern void     vsym_set_sp_offset(var_sym *, int);
extern int      vsym_get_sp_offset(var_sym *);
extern void     vsym_update_usage(var_sym *);   /* usage count methods */
extern int      vsym_used(var_sym *);           /* returns usage count */
extern void     vsym_set_preg(var_sym *, int);  /* parameter reg methods */
extern boolean  vsym_passed_in_preg(var_sym *);
extern int      vsym_get_preg(var_sym *);
extern sym_addr vsym_get_auto_sym(base_symtab *, int, int);

#endif /* ANNOTEHELPER_H */
