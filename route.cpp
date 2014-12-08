/*
 * route.cpp
 *
 *  Created on: 2014年5月29日
 *      Author: qmwx
 */

#include <stdio.h>
#include "route.h"
#include "common.h"
#include "super_block_cfg.h"

extern super_block_cfg *the_scfg; /*super block cfg composed of super block */



route::route(int from, int to):from_block(from), to_block(to),
		most_likely_path(NULL), all_possible_path(NULL), all_suit_path(NULL)
{
	find_all_possible_path();
}

/**
 * 怎么找到多条路径?
 */
void
route::find_all_possible_path()
{
	super_block *from_sbock = get_from_block();
	super_block *to_sblock = get_to_block();
	assert(from_sbock && to_sblock);

	all_possible_path = new std::vector<super_block_path*>;

	super_block_path * the_path = new super_block_path();
	super_block *start_sblock = from_sbock->less_likely_succ();
	super_block *the_sblock = start_sblock;
	while(the_sblock != to_sblock){
		the_path->add_super_block(the_sblock);
		the_sblock = the_sblock->likely_succ();
	}

	all_possible_path->push_back(the_path);
	return ;
}


void
route::filter_sp_block_path(int sp_num)
{
}



void
route::print_all_possible_path_block_num(FILE* fp)
{
}


void
route::print_all_suit_path_block_num(FILE* fp)
{
}

super_block*
route::get_from_block() const
{
	assert(the_scfg);
	return the_scfg->get_super_block_by_num(from_block);
}

super_block*
route::get_to_block() const
{
	assert(the_scfg);
	return the_scfg->get_super_block_by_num(to_block);
}

route::
~route()
{
	delete all_possible_path;
	delete all_suit_path;
}

void
route::print(FILE *fp)
{
	fprintf(fp, "in route print method\n");
	return ;
}


