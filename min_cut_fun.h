/*
 * min_cut_fun.h
 *
 *  Created on: 2012-2-16
 *      Author: harry
 */

#ifndef MIN_CUT_FUN_H_
#define MIN_CUT_FUN_H_

#include "super_block_path.h"
#include "super_block_cfg.h"
#include "super_block.h"
#include "node.h"

#include <vector>


/**
 * super_block_path is constructed by num_vec
 */
super_block_path *get_super_block_path_by_num_vec(const std::vector<int> &num_vec, const super_block_cfg *scfg);

/**
 * @param scfg
 * @return vector<int> - contains all the super_block num on the most likely Path of whole procedure
 */
std::vector<int> get_num_vec_from_scfg(const super_block_cfg *scfg);
std::vector<int> get_possible_path_num_vec_from_scfg(const super_block_cfg *scfg, int& with_new_path);

/**
 * @param scfg
 * @return vector<int> - contains all the super_block num on likely_Path
 */
std::vector<int> get_num_vec_from_super_block(const super_block *likely_path, const super_block_cfg *scfg);


/**
 * @param da_cfg
 * @param num_vec - vector of super_block num
 * @return int    - calculate the number of instructions
 */
int get_node_group_size(const std::vector<int> &num_vec, da_cfg *cfg);

/**
 * 根据num_vec构造super_block::BLOCK_LIST，
 * 对于循环只选取most_likely_pah
 */
super_block *get_super_block_list_by_vec(const std::vector<int> num_vec, da_cfg *cfg);

/**
 * 根据path_related确定最后最后一次被定值的循环节点，
 * 根据得到的起始点sp,两者间选靠后的
 * 然后将前面的节点剪切掉，重构spath.
 * param[out] sp;
 * param[in] spath
 * return super_block - new super_block path
 */
super_block* reconstruct_spath_by_related_loop(super_block *spath, bit_set *path_related, tnle* &sp, const super_block_cfg *scfg);


/**
 * 根据path_related确定最后最后一次被定值的循环节点，
 * 然后，返回循环之后的第一个cfg_node
 */
cfg_node * get_cfg_node_after_ralated_loop(const super_block *spath, bit_set *path_related, const super_block_cfg *scfg);


/**
 * @descript - 获取src_cnl中以start_cn开始的子cfg_node_list
 * @param[in] src_cnl
 * @param[in] start_cn
 * @others  - if cn is not contains in src_cnl, return copy(src_cnl)
 */
cfg_node_list *sub_cnl(const cfg_node_list *src_cnl, const cfg_node *start_cn);



/**
 * @descript - 获取src_cnl中end_cn之前的子cfg_node_list
 * @param[in] src_cnl
 * @param[in] end_cn
 * @others  - if cn is not contains in src_cnl, return copy(src_cnl)
 */
cfg_node_list *sub_cnl_front(const cfg_node_list *src_cnl, const cfg_node *end_cn);


/**
 * @descript - 根据tnle分割src_cnl，取后部分
 * @param[in] src_cnl
 * @param[in] te
 * @others  - assert(), src_cnl contains te
 */
cfg_node_list *sub_cnl_after_tnle(const cfg_node_list *src_cnl, tnle *te);








//---------------------------------------for test -----------------------------------
/**
 * 1(size), 2(size), 3(size),  4(size), 5(size), 6(size), 7(size)
 */
void test_print_num_vec(FILE *fp, const std::vector<int> num_vec, da_cfg *cfg);


/**
 * [1,2,3], (4),  [5, 6, 7],  (8)
 * 注意：循环处理过程中scfg还没有构造出来，此函数不能在循环处理中调用
 */
void test_print_node_list(FILE *fp, const util::node_list  *head, da_cfg *cfg);

/**
 * [1,2,3]:(0x0), [5, 6, 7]_(0xfffff)
 */
void test_print_node_with_sp(FILE *fp, const util::node_list  *head, const super_block_cfg *scfg);


void test_print_vec(FILE *fp, const std::vector<double> vec);




#endif /* MIN_CUT_FUN_H_ */
