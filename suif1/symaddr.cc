/*  Symbolic Address Implementation */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#define _MODULE_ "libsuif.a"

#pragma implementation "symaddr.h"

#define RCS_BASE_FILE symaddr_cc

#include "suif1.h"

RCS_BASE(
    "$Id: symaddr.cc,v 1.1.1.1 1998/06/16 15:15:09 brm Exp $")


void
sym_addr::print (FILE *f)
{
    putc('<', f);
    symbol()->print(f);
    fprintf(f, ",%d>", offset());
}


sym_addr::sym_addr (in_stream *is, base_symtab *symtab)
{
    sym = sym_node::read(is, symtab);
    off = is->read_int();
}


void
sym_addr::write (out_stream *os)
{
    symbol()->write(os);
    os->write_int(offset());
}


