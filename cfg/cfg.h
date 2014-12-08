/*  Top-level Control Flow Graph Include File */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995, 1996, 1997 The President and Fellows of Harvard 
    University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>


#ifndef CFG_H
#define CFG_H

/*
 *  Use a macro to include files so that they can be treated differently
 *  when compiling the library than when compiling an application.
 */

#ifdef CFGLIB
#define CFGINCLFILE(F) #F
#else
#define CFGINCLFILE(F) <cfg/ F >
#endif

#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(CFGLIB)
#define EXPORTED_BY_CFG _declspec(dllimport) extern
#else
#define EXPORTED_BY_CFG extern
#endif

#include CFGINCLFILE(graph.h)
#include CFGINCLFILE(node.h)
#include CFGINCLFILE(util.h)

#endif /* CFG_H */

