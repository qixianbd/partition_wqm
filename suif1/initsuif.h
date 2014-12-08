/*  Library Initialization */

/*  Copyright (c) 1994, 1997, 1998 Stanford University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef INITSUIF_H
#define INITSUIF_H

#pragma interface

RCS_HEADER(initsuif_h,
    "$Id: initsuif.h,v 1.2 1999/08/25 03:29:11 brm Exp $")

/*
 *  Library initialization.  For the SUIF library to be able to print the
 *  version numbers for all the libraries linked in a particular program,
 *  it needs to know which libraries are included.  Thus, the user must
 *  register each library (using the "register_library" function) before
 *  calling "init_suif".  The library records the information for the
 *  registered libraries in a list of "suif_library" structures.  The
 *  "start_suif" function declared here may be automatically generated
 *  by the makefiles to register the appropriate libraries.
 */

typedef void (*lib_init_f)(int& argc, char *argv[]);
typedef void (*lib_exit_f)();

struct suif_library {
    char *name;				/* name of the library (e.g. "suif") */
    char *version;			/* the library version */
    char *compile_info;			/* who/when/where was it compiled */
    lib_init_f initializer;		/* function to initialize it */
    lib_exit_f finalizer;		/* function to deallocate storage */
};

DECLARE_DLIST_CLASS(library_list, suif_library*);
EXPORTED_BY_SUIF library_list *suif_libraries;	/* list of registered libraries */

void register_library(char *nm, char *ver, char *info,
		      lib_init_f init_f=NULL,
		      lib_exit_f exit_f=NULL);
void start_suif(int& argc, char *argv[]);
void init_suif(int& argc, char *argv[]);
#define exit_suif exit_suif1
extern "C" void exit_suif1(void);

#define LIBRARY(name, init_f, exit_f) \
    EXPORTED_BY_SUIF char *lib ## name ## _ver_string; \
    EXPORTED_BY_SUIF char *lib ## name ## _who_string; \
    EXPORTED_BY_SUIF void init_f(int& argc, char *argv[]); \
    EXPORTED_BY_SUIF void exit_f(); \
    register_library(#name, lib ## name ## _ver_string, \
		     lib ## name ## _who_string, init_f, exit_f)

/* these variables are linked in by the standard Makefiles */
extern char *prog_ver_string;		/* program version, e.g. "2.4" */
extern char *prog_who_string;		/* who, when, where compiled/linked */
extern char *prog_suif_string;		/* SUIF distribution version */
#define libsuif_ver_string libsuif1_ver_string
#define libsuif_who_string libsuif1_who_string
#define libsuif_suif_string libsuif1_suif_string
extern char *libsuif1_ver_string;
extern char *libsuif1_who_string;
extern char *libsuif1_suif_string;

extern "C" void enter_suif1(int *argc, char *argv[]);
extern "C" void register_suif1(void);

#endif /* INITSUIF_H */
