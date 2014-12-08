#ifndef ST_MISC_H
#define ST_MISC_H

#include "common.h"
#include "super_block_cfg.h"
#include <suif_copyright.h> 
#include <suif1.h>


void print_bit_man(operand_bit_manager *the_bit_mgr, FILE *fp = stdout);

void print_all_definee(FILE *fp, reaching_def_problem *reach, const super_block_cfg *scfg);

double branch_taken_probability(cfg_node *cn);

int get_branch_node_execute_times(cfg_node *cn);

tree_instr* 
search_latest_define_point(operand var_op, tree_instr *pos, cfg_block *curr_block, super_block *subgraph);

bit_set* 
search_latest_define_point(bit_set *definees, super_block *subgraph, bit_set *res);

tree_instr*
search_latest_def_point_before_instr(operand var_op, tree_instr *pos, cfg_block *curr_block, super_block *subgraph);

cfg_block*
find_block_contains_instr(super_block *subgraph, machine_instr *mi);

bit_set*
search_related_point(bit_set *definees, super_block *subgraph, bit_set *related);


/*
 *search_related_point_before_instr() --- find the definee's related live-ins
 */
bit_set *search_related_point_before_instr(bit_set *live_in, super_block *subgraph,machine_instr *cqip_instr, bit_set *related);


/**
 * 为m_instr的操作数寻找定义点
 */
void seach_related_point_for_opd(machine_instr *m_instr, cfg_block *cb,
		super_block *subgraph, bit_set *& res);

extern void enroll_variables(tree_block *, operand_bit_manager *);

/**
 * get immdediate predecussor tnle* by current tnle
 * pos不是块内第一条指令
 */
machine_instr *get_immediate_pre_instr_in_block(machine_instr *other_mi);


#endif
