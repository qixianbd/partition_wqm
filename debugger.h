/*
 * debugger.h
 *
 *  Created on: 2012-2-28
 *      Author: harry
 */

#ifndef DEBUGGER_H_
#define DEBUGGER_H_

#include <cstdio>

namespace debugger{



void enter_proc(const char *proc_name, FILE *fp);

void exit_proc(const char *proc_name, FILE *fp);

void arrive_here(const char *proc_name, const char *place, FILE *fp);

void print_value(const char *val_name, const char* val, FILE *fp);

void print_value(const char *val_name, const int iVal, FILE *fp);

}


#endif /* DEBUGGER_H_ */
