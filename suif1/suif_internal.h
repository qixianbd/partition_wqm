/*  File "suif_internal.h" */

/*  Copyright (c) 1995 Stanford University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#ifndef SUIF_INTERNAL_H
#define SUIF_INTERNAL_H

#ifndef SUIFLIB
#error "for the SUIF library only"
#endif

#include <suif_copyright.h>
#include "suif1.h"

RCS_HEADER(suif_internal_h,
    "$Id: suif_internal.h,v 1.1.1.1 1998/06/16 15:15:03 brm Exp $")


/*
 *  This file is for information that is global to different parts of
 *  the SUIF library but private to the library.
 */

extern base_symtab *write_scope;
extern boolean show_error_context;
extern boolean warn_only_once;

extern void init_error_handler(void);
extern boolean clue_stack_empty(void);

#endif /* SUIF_INTERNAL_H */
