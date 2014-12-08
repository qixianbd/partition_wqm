/* Implementation of class mapping instructions to definition points */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

char *k_def_point_1st;
char *k_def_point_num;

def_catalog::def_catalog()
{
    number = 0;
    table = new instruction_array;
}

def_catalog::~def_catalog()
{
    for (int i = 0; i < num_defs(); i++) {
        (*table)[i]->get_annote(k_def_point_1st);
        (*table)[i]->get_annote(k_def_point_num);
    }

    delete table;
}

int def_catalog::num_defs()
{
    return number;
}

int def_catalog::enter(instruction *instr, int count)
{
    if (!count)
        return -1;

    int first = first_id(instr);
    if (first >= 0)
        return first;

    int *ptr_1st = new int;
    int *ptr_num = new int;

    *ptr_1st = number;
    *ptr_num = count;
    instr->prepend_annote(k_def_point_1st, (void *)ptr_1st);
    instr->prepend_annote(k_def_point_num, (void *)ptr_num);

    do {
        table->extend(instr);
        number++;
    } while (--count);

    return *ptr_1st;
}

instruction *def_catalog::lookup(int index)
{
    return ((unsigned)index >= (unsigned)num_defs()) ? NULL : (*table)[index];
}

int def_catalog::first_id(instruction *instr)
{
    void *data = instr->peek_annote(k_def_point_1st);

    return data ? *((int *)data) : -1;
}

int def_catalog::num_ids(instruction *instr)
{
    void *data = instr->peek_annote(k_def_point_num);

    return data ? *((int *)data) : -1;
}
