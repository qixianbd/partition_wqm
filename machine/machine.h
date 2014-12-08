/* file "machine.h" */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

/*  Header for SUIF library of machine-specific definitions and routines */

#ifndef MACHINE_H
#define MACHINE_H

// Mark exported symbols as DLL imports for Win32. (jsimmons)
#if defined(_WIN32) && !defined(__CYGWIN32__) && !defined(MACHINELIB)
#define EXPORTED_BY_MACHINE _declspec(dllimport) extern
#else
#define EXPORTED_BY_MACHINE extern
#endif

/* 
 *  Use a macro to include files so that they can be treated differently
 *  when compiling the library than when compiling an application.
 */

#ifdef MACHINELIB
#define MACHINEINCLFILE(F) #F
#else
#define MACHINEINCLFILE(F) <machine/ F >
#endif

/*
 *  The files are listed below in groups.  Each group generally depends
 *  on the groups included before it.  Within each group, indentation is
 *  used to show the dependences between files.
 */

/* this should always be defined first */
#include <suif1.h>

/* annotation, target-architecture, and effective-address helper routines */
#include MACHINEINCLFILE(annoteHelper.h)
#include MACHINEINCLFILE(archInfo.h)
#include MACHINEINCLFILE(eaHelper.h)

/* machine instruction base classes */
#include MACHINEINCLFILE(machineDefs.h)
#include   MACHINEINCLFILE(machineInstr.h)

#ifdef M_ALPHA
#include   MACHINEINCLFILE(alphaOps.h)
#include     MACHINEINCLFILE(alphaInstr.h)
#endif

#ifdef M_MIPS
#include   MACHINEINCLFILE(mipsOps.h)
#include     MACHINEINCLFILE(mipsInstr.h)
#endif

#ifdef M_PPC
#include   MACHINEINCLFILE(ppcOps.h)
#include     MACHINEINCLFILE(ppcInstr.h)
#endif

#ifdef M_X86
#include   MACHINEINCLFILE(x86Ops.h)
#include     MACHINEINCLFILE(x86Instr.h)
#endif

/* code gen/query helper -- depends upon archInfo.h and machineDefs.h */
#include MACHINEINCLFILE(machineUtil.h)

#endif /* MACHINE_H */
