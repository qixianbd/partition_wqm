/*
 * loop_cut.h
 *
 *  Created on: 2012-3-16
 *      Author: harry
 */

#ifndef LOOP_CUT_H_
#define LOOP_CUT_H_

#include "common.h"
#include "loop_block.h"
#include "node.h"

loop_block *partition_loop(cfg_node * loop_entry);

loop_block *construct_loop(cfg_node * loop_entry);


/**
 * spawn between iterators
 */
void partition_curr_loop(loop_block *loop);


/**
 * 根据loop_likely_path上的内部循环，将loop_likely_path_vec分裂成多个natural_groups
 */
util::node_list *split_loop_likely_path_by_loop(const std::vector<int> &loop_likely_path_vec);


/**
 * 对已经获得sp和cqip的loop进行构造线程工作
 */
void construct_loop_thread(loop_block *loop);


/**
 * find best cqip_pos and spawn_pos for loop
 * @param[in]  loop
 * @param[out] cqip_pos
 * @param[out] spawn_pos
 * @param[out] pslice_dept -相关点
 */
bool get_best_loop_cqip(loop_block *loop, tnle *&cqip_pos, tnle *&spawn_pos, bit_set *pslice_dept);


/**
 * find best cqip_pos and spawn_pos for loop
 * @param[in]  loop
 * @param[out] cqip_pos
 * @param[out] spawn_pos
 */
bool get_best_cqip_for_loop_with_innerloop(loop_block *loop, tnle *&cqip_pos, tnle *&spawn_pos,  bit_set *pslice_dept);


/**
 * @param[in]  spawn_sb
 * @param[in]  cqip_instr
 * @param[in]  live_ins
 * @param[in] cqip_pos
 * @param[out] spawn_pos
 * @param[out] pslice_dept ---依赖指令，最后直接作为pslice
 */
int get_best_sp_at_given_cqip(super_block *spawn_sb, machine_instr *cqip_instr,
		bit_set *live_ins, tnle *cqip_pos, tnle *&spawn_pos, bit_set *pslice_dept);


/**
 * find best cqip_pos and spawn_pos for loop
 * @param[in]  loop
 * @param[out] cqip_pos
 * @param[out] spawn_pos
 * @param[out] instr_dept
 */
bool get_best_cqip_for_loop_without_innerloop(loop_block *loop, tnle *&cqip_pos, tnle *&spawn_pos, bit_set *pslice_dept);




/**
 * @description - 用于处理hasInnerLoop的循环，
 *                输入的是根据innerLoop裁剪的最前面的一段nxt_cqip_path和最后一段biggest_spawn_path
 *                根据related_call裁剪biggest_spawn_path
 */
super_block *prunged_loop_path_by_pos_trace(super_block *biggest_spawn_path, super_block *nxt_cqip_path);

/**
 * @description - 通过对loop_likely_path
 *                根据related_loop裁剪
 */
super_block *prunged_loop_path_by_inner_loop(loop_block *loop);


/**
 * 返回loop的most_likely_path的开始节点到第一个innerLoop前的节点
 */
super_block *get_non_innerloop_cqip_path(loop_block *loop) ;

//----------------------test----------------------------
void test_print_loop_end(FILE *fp, cfg_block *blk);



#endif /* LOOP_CUT_H_ */
