/*
 * min_cut.h
 *
 * using algorithm based on min cut to partition the non-loop region
 *
 *  Created on: 2011-12-22
 *      Author: harry
 */

#ifndef MIN_CUT_H_
#define MIN_CUT_H_

#include "super_block_cfg.h"
#include "node.h"

#include <vector>


/**
 * @description -
 *               (0).初始化-得到一个节点组
 *               (1).按照循环自然分割——节点分裂
 *               (2).节点组分裂
 *               (3).重新计算sp点
 * @param scfg - the super_block_cfg of the procedure,  readonly
 * @return node_list 单链表结构，每一个node表示一个节点组
 */
void non_loop_partition(super_block_cfg *scfg);
void non_loop_partition_first_pass(super_block_cfg *scfg);


/**
 * 根据循环，将num_vec分裂成多个节点组，num_ve已经被处理过了(不包含entry和exit节点)
 * 初始使用，减小初始规模
 */
util::node_list *split_node_group_by_loop(const std::vector<int> & num_vec,
		super_block_cfg * scfg);


/**
 * 节点组分裂过程
 * 遍历node_list过程，对每个节点组根据粒度进行划分
 * 中间结果统计见 麦库
 */
void split_nloop_region(util::node_list * &natual_node_groups, da_cfg *cfg);




/**
 * @description - 以natual_node_group为单位进行穷举划分
 *   1.划分
 *   2.寻找sp点
 * @others 先整体划分完毕，再寻找sp点，因为后一步划分会影响前一步划分的数据依赖
 */
void partition_natual_node_group(const std::vector<int> &natual_group,
		da_cfg *cfg);


util::node_list *split_natural_node_group(const std::vector<int> & natual_group, da_cfg * cfg);

/**
 * @descript - evalue all the edges in this node group and choose the best one
 * @param num_vec - node group and it's size is big
 * @param natual_group - 划分的基本单位，num_vec是其子集，用来寻找spawn_path
 * @return int    - pos to split the vector into two pieces
 *                  if found, return the index of elem in vector
 *                  else return -1;
 *                  assert(pos == -1 || pos > 0);
 *                  num_vec[index]为cqip节点
 */
int get_opt_split_pos(const std::vector<int> &num_vec, const std::vector<int> &natual_group, da_cfg *cfg);


/**
 * 计算在num_vec上npos上分割时，评估值
 *                如果边不能分割，对应评估值取-1。
 * @param npos -  npos表示边的后一个节点。
 * @param edge_evalue_vec - 存放评估值，便于测试和获取中间数据
 *           edge_evalue_vec[0]为0.0，没有使用，
 *           num_vec[npos]为对应cqip点的第一个节点
 * @param natual_group - 用于获取激发路径
 * @others   - 使用全局变量the_scfg、 the_cfg、 the_bit_manager、the_reach
 */
double evalue_at_given_pos(const std::vector<int> &num_vec, int npos, std::vector<double> &edge_evalue_vec, const std::vector<int> &natual_group, da_cfg * cfg);

//2012修改，使得不能跨越循环
/**
 * @description - 获取有效路径
 * @param cqip_vec - 不允许为empty
 * @param natual_group,作为最大激发距离
 */
const std::vector<int> get_effect_spawn_path_vec(
		const std::vector<int> & cqip_vec, const std::vector<int> &natual_group, da_cfg *cfg);

/**
 * destruct all the element in nl.
 */
//void destruct_snode_list(node_list* nl);


/**
 * @descript - 根据spawn_pos获取cqip_vec对应线程的激发起点
 * spawn_trace 用来分析函数调用点，我们不希望将(定值相关的函数调用指令)包含在预计算片段中,
 * 因此，希望从最后一条(定值相关的函数调用指令)之后开始
 * 由于super_block建立时设置break_at_call = true, 所以基本块满足end_in_call，所以直接从一下块开始
 * @others  - 至于为什么 spawn_pos_trace 不包含循环，这是由于历史原因造成的，为了兼容，我也也沿袭
 */
const std::vector<int> get_spawn_vec_by_spawn_trace(const std::vector<int> &biggest_path_vec,
		const std::vector<int> & cqip_vec, da_cfg *cfg);




/**
 * @description 根据spawn_vec上loop分割，取后端
 * @others spawn_vec可以为empty
 * @ret 可以为empty
 */
const std::vector<int> spawn_vec_back_split_by_loop(
		const std::vector<int> &spawn_vec, const std::vector<int> &cqip_vec,
		da_cfg *cfg);

/**
 * @description - 返回spawn_vec中的最后一个loop的num，不存在则返回-1
 * @ret num if exited, else -1
 */
int get_last_loop_num(const std::vector<int> & spawn_vec, da_cfg *cfg);

/**
 * 直接调用get_best_evaluation_and_spawn_pos
 * @param spawn_vec 允许empty
 * @param cqip_vec  不允许为empty
 */
double get_best_evaluation(const std::vector<int> &spawn_vec, const std::vector<int> &cqip_vec, da_cfg *cfg);

/**
 * 为给定的spawn_vec和cqip_vec寻找sp点
 * 如果没有找到，则返回NULL
 */
tnle *get_spawn_pos(const std::vector<int> & cqip_vec, const std::vector<int> &natual_group,
		da_cfg *cfg);

/**
 *
 * 在spawn_vec中寻找sp点，寻找过成先要满足一定的约束条件，又要使评价函数值最大化
 * 否则返回 < 0
 * @param spawn_vec - 可以为empty
 */
double get_best_evaluation_and_spawn_pos(const std::vector<int> & spawn_vec,
		const std::vector<int> & cqip_vec, tnle *&spawn_pos,
		da_cfg *cfg);


/**
 * 为head中的所有非Loop的node_group寻找和设置sp点
 * 激发路径在natual_group中
 */
void set_sp_for_all_node_group(util::node_list *head, const std::vector<int> &natual_group, da_cfg *cfg);


/**
 * set thread
 */
void construct_nloop_thread(util::node_list * head, const super_block_cfg *scfg);

void destruct_node_list(util::node_list * &head);

//---------------------------discard----------------------------------
/**
 * @description 返回最后一个related_loop的num, 如果不存在，返回-1
 *              related_loop表示，有包含定值点的loop
 *
 * @others    使用全局变量  reaching_def_problem *the_reach
 */
int get_last_related_loop_num(const std::vector<int> &spawn_vec, bit_set *related, const super_block_cfg *scfg);



/**
 * @description - 根据relted_loop裁剪spawn_vec
 *                检测所有（定值点和相关点ralated_）,如果存在loop中，则选取loop之后的子路径返回
 *                因为在loop中的定值点我们不能预测
 * @ret           返回裁剪后的spawn_vec - 可能为empty
 */
//const std::vector<int> pruned_spawn_vec_by_related_loop(const std::vector<int> &spawn_vec, const std::vector<int> &cqip_vec, const super_block_cfg *scfg);



///----------------------------------loop_body_split---------------------------------///
//由于loop_body划分不能使用scfg，所以，我需要再提供一份使用da_cfg的版本
///----------------------------------------------------------------------------------///
////////////////////////////////////////////////////////////////////////////////////////////////////



#endif /* MIN_CUT_H_ */
