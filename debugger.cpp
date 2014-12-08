/*
 * debugger.cpp
 *
 *  Created on: 2012-2-28
 *      Author: harry
 */

#include "debugger.h"
#include <cassert>
#include <cstdio>

namespace debugger{

void enter_proc(const char *proc_name, FILE *fp)
{
	assert(proc_name != NULL);
	fprintf(fp, "<--Enter %s().--\n", proc_name);
	fflush(fp);
}



void exit_proc(const char *proc_name, FILE *fp)
{
	assert(proc_name != NULL);
	fprintf(fp, "--Exit %s().-->\n\n", proc_name);
	fflush(fp);
}

void arrive_here(const char *proc_name, const char *place, FILE *fp)
{
	assert(proc_name != NULL);
	assert(place != NULL);
	fprintf(fp, "\t[%s]-%s\n", proc_name, place);
	fflush(fp);
}

void print_value(const char *val_name, const char *val, FILE *fp)
{
	assert(val_name != NULL);
	assert(val != NULL);
	fprintf(fp, "\t%s = %s.\n", val_name, val);
	fflush(fp);
}


void print_value(const char *val_name, const int iVal, FILE *fp)
{
	assert(val_name != NULL);
	fprintf(fp, "\t%s = %d.\n", val_name, iVal);
	fflush(fp);
}

}



