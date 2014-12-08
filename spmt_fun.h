/*
 * spmt_fun.h
 *	functions that moved here from main.cpp
 *  Created on: 2012-3-12
 *      Author: harry
 */

#ifndef SPMT_FUN_H_
#define SPMT_FUN_H_

#include "super_block.h"
#include "thread.h"
#include <vector>




/** for min_cut.h and main.cpp
 * @param begin - super block*
 * @param end - super block*
 * @return super_block_path -  most likely path in regin (begin, end)
 */
super_block_path *find_most_likely_path(super_block * begin, super_block * end);

/**
 * [begin, end]
 */

super_block_path *find_most_likely_path_include_two_ends(super_block * begin,
		super_block * end);



/**
 * @description - 计算spawn_instr到块cqip的第一条指令的距离
 *                其中cqip为spawn_path的直接后继
 */
unsigned int compute_spawning_distance_from_sp(super_block * spawn_path,
		machine_instr * spawn_instr);



/**
 * 计算[spawn_instr, cqip_instr)的距离
 */
unsigned int compute_spawning_distance_between_sp_cqip(super_block *spawn_path,
		machine_instr *spawn_instr, machine_instr *cqip_instr);

/*
 *pruning_instrs_before_spawn() --- prune the latest define point of the instructions before spawn position
 裁剪spawn点之前的指令的最近定义点，这个函数的输入是定义点的指令，不是所有的指令
 输入发起线程的两个cqip点之间的所有的定义点指令，然后将spawn点之前的定义点指令去除
 */
void pruning_instrs_before_spawn(bit_set * defs, super_block * path, tnle * spawn_pos);


void finish_construction(thread * curr_thread);


void add_thread(std::vector<thread *>*threads, thread * curr_thread);


bool is_existed_curr_thread(std::vector<thread *>*threads,
				thread * curr_thread);

tree_node_list *construct_pslice(bit_set * ins_dep, label_sym * cqip_pos,
		tree_node_list * pslice);

super_block *find_pslice_path(super_block * spawn_sblock,
		super_block * cqip_sblock);



bit_set *search_pslice_instrs(bit_set * liveins, super_block * spawn_path,
		tnle * spawn_pos, bit_set * defs);



void pruning_cal_instrs(bit_set * defs, super_block * spawn_path);



/**
 * addbykeyming 20140623
 */
super_block_path *find_most_likely_path_with_sp_block(super_block * begin, super_block * end);

std::vector<super_block_path*>* find_most_likely_multi_path(super_block * begin, super_block * end);


#endif /* SPMT_FUN_H_ */
