/*
 * min_cut_fun.cpp
 *
 *  Created on: 2012-2-16
 *      Author: harry
 */

#include "spmt_fun.h"
#include "min_cut_fun.h"
#include "loop_block.h"
#include "da_cfg.h"

#include <iostream>
#include <algorithm>





/**
 * super_block_path is constructed by num_vec
 */
super_block_path *get_super_block_path_by_num_vec(const std::vector<int> &num_vec, const super_block_cfg *scfg)
{
	assert(!num_vec.empty());
	super_block_path *path = new super_block_path();

	super_block *sb = NULL;
	for (unsigned i = 0; i < num_vec.size(); ++i) {
		sb = scfg->get_super_block_by_num(num_vec[i]);
		path->add_super_block(sb);
	}

	return path;
}




/**
 * +[测试通过]
 * @param scfg
 * @return vector<int> - contains all the super_block num on the most likely Path of whole procedure
 *                       not contain the entry and exit node
 */
std::vector<int> get_num_vec_from_scfg(const super_block_cfg *scfg) {
	std::vector<int> num_vec;

	super_block *entry_block = scfg->entry();
	super_block *start_block = entry_block->fall_succ();
	super_block *end_block = scfg->exit();

	/**
	 * get super_block num on the most likely_path and put them into the node_list.
	 */
	if (start_block == end_block ) {
		return num_vec;
	}

	super_block *pdom = NULL;
	super_block_path *likely_path = NULL;
	super_block_list* sb_list = NULL;
	super_block *sb = NULL;

	num_vec.push_back(start_block->block_num());
	while (start_block != end_block) {
		pdom = start_block->immed_pdom();

		likely_path = find_most_likely_path(start_block, pdom); //likely_path不包含头和尾
		sb_list = likely_path->super_blocks();
		super_block_list_iter sbl_iter(sb_list);
		while (!sbl_iter.is_empty()) {
			sb = sbl_iter.step();
			num_vec.push_back(sb->block_num());
		}

		if(pdom != end_block){
			num_vec.push_back(pdom->block_num());
		}
		start_block = pdom;
	}

	return num_vec;
}

/**
 * addbykeyming 20140530
 * not completed.
 * @param scfg
 * @return
 */
std::vector<int> get_possible_path_num_vec_from_scfg(const super_block_cfg *scfg, int& with_new_path)
{
	std::vector<int> ret_vec;

	// 1. 找出第一遍划分后的sp_block对应的所有互斥block. 姑且称这样的 block 为opt_sp_block.

	// 1.1 找到之前的sp_block

	// 1.2 判断之前的对每一个sp_block, 找到对应的opt_sp_block.


	// 2. 为每一个opt_sp_block 找到其对应的opt_cqip_block.

	//3. 对于每一对 opt_sp_block 和  opt_cqip_block, 找到可能路径, opt_likely_path. 出入可能路径链表 ret_vec. 返回ret_vec.

	std::vector<int> num_vec;

	super_block *entry_block = scfg->entry();
	super_block *start_block = entry_block->fall_succ();
	super_block *end_block = scfg->exit();

	/**
	 * get super_block num on the most likely_path and put them into the node_list.
	 */
	if (start_block == end_block ) {
		return num_vec;
	}

	super_block *pdom = NULL;
	super_block_path *likely_path = NULL;
	super_block_list* sb_list = NULL;
	super_block *sb = NULL;

	num_vec.push_back(start_block->block_num());
	while (start_block != end_block) {
		pdom = start_block->immed_pdom();

		/**
		 * 对于每一个pdom, 判断其是否有是对应的sp_block的dom block, 如果是, 进行特殊处理
		 */

	//	if(std::find(scfg->get_sp_post_dom_list().begin(), scfg->get_sp_post_dom_list().end(), pdom->block_num()))

		int sp_block_flag = 0;
		for(int ipos = 0; ipos < scfg->get_sp_post_dom_list().size(); ipos++){
			if(start_block->block_num() == scfg->get_sp_post_dom_list()[ipos]){
				int start_block_num = start_block->block_num();
				if(start_block_num != 0 && scfg->is_in_sp_block_map(start_block_num)){
					/*num_vec.clear();
					return num_vec;*/
					sp_block_flag = 0;
					break;
				}


				likely_path = find_most_likely_path_with_sp_block(start_block, pdom);
				if(likely_path == NULL){
					/*num_vec.clear();
					return num_vec;*/
					sp_block_flag = 0;
					break;
				}
				sp_block_flag = 1;
				with_new_path++;
				break;
			}
		}


		if(sp_block_flag == 0){
			likely_path = find_most_likely_path(start_block, pdom); //likely_path不包含头和尾
			//with_new_path = 0;
		}

		sb_list = likely_path->super_blocks();
		super_block_list_iter sbl_iter(sb_list);
		while (!sbl_iter.is_empty()) {
			sb = sbl_iter.step();
			num_vec.push_back(sb->block_num());
		}

		if(pdom != end_block){
			num_vec.push_back(pdom->block_num());
		}
		start_block = pdom;
	}

	return num_vec;
}


/**
 * @param scfg
 * @return vector<int> - contains all the super_block num on likely Path
 */
std::vector<int> get_num_vec_from_super_block(const super_block *likely_path, const super_block_cfg *scfg){
	assert(NULL != likely_path);

	std::vector<int> num_vec;
	cfg_node_list_iter iter(likely_path->nodes());
	while(!iter.is_empty()){
		int num = iter.step()->number();
		num_vec.push_back(num);
	}

	return num_vec;
}




/**
 * @param da_cfg
 * @param num_vec - vector of super_block num
 * @return int    - calculate the number of instructions
 */
int get_node_group_size(const std::vector<int> &num_vec, da_cfg *cfg){
	assert(num_vec.size() > 0);
	int sum = 0;
	for (int i = 0; i < num_vec.size(); ++i) {
		sum += cfg_node_static_instr_size(cfg->node(num_vec[i]));
	}
	return sum;
}


/**
 * 根据num_vec构造super_block::BLOCK_LIST，
 * 对于循环只选取most_likely_pah
 * 由于我们的策略，使得num_vec中不包含循环
 */
super_block *get_super_block_list_by_vec(const std::vector<int> num_vec, da_cfg *cfg){
	assert(num_vec.size() > 0);

	super_block *nodes = new super_block(super_block::BLOCK_LIST);
	for (int i = 0; i < num_vec.size(); ++i) {
		cfg_node *cn = cfg->node(num_vec[i]);
		nodes->add_block(cn);
		if(i > 0){
			assert(!cfg->is_loop_begin(num_vec[i]));//assert
		}
	}

	return nodes;
}


/**
 * 根据path_related确定最后最后一次被定值的循环节点，
 * 根据得到的起始点sp,两者间选靠后的
 * 然后将前面的节点剪切掉，重构spath.
 * param[out] sp;
 * param[in] spath
 * return super_block - new super_block path
 */
super_block* reconstruct_spath_by_related_loop(super_block *spath,
		bit_set *path_related, tnle* &sp, const super_block_cfg *scfg) {
	cfg_node *cn = get_cfg_node_after_ralated_loop(spath, path_related, scfg);
	/**
	 * 需要判断cn == NULL和sp==NULL的情况
	 */

	if (cn == NULL && sp == NULL) {
		return spath;
	}
	//直接在cnl上面操作，根据cn提取后为new_cnl
	cfg_node_list *cnl = spath->nodes();
	cfg_node_list *new_cnl = sub_cnl(cnl, cn);

	//在new_cnl上面操作，提取dst_cnl
	cfg_node_list *dst_cnl = NULL;
	cfg_node *sp_cn = NULL;
	if (NULL != sp) {
		sp_cn = scfg->in_which_cfg_node(sp);
		if (NULL != new_cnl->lookup(sp_cn)) {
			//sp不变，截断
			dst_cnl = sub_cnl(new_cnl, cn);
			delete new_cnl;
		}
	} else {
		dst_cnl = new_cnl;
		cfg_node *begin_cn = dst_cnl->head()->contents;
		if (begin_cn->is_block()) {
			sp = ((cfg_block *) begin_cn)->in_head()->list_e();
		} else if (begin_cn->is_instr()) {
			sp = ((cfg_instr *) begin_cn)->instr()->list_e();
		} else {
			assert(false);
		}
	}

	//delete spath and re-construct spath
	super_block *spawn_path = new super_block();

	super_block *first_sblock = scfg->in_which_super_block(
			dst_cnl->head()->contents);
	super_block *end_sblock = scfg->in_which_super_block(
			dst_cnl->tail()->contents);

	super_block_path *new_path = find_most_likely_path_include_two_ends(
			first_sblock, end_sblock);
	super_block *path = new_path->convert_to_super_block();
	spawn_path->add_blocks(path->nodes());

	delete spath;
	spath = NULL;
	return spawn_path;
}



/**
 * 根据path_related确定最后最后一次被定值的循环节点，
 * 然后，返回循环之后的第一个cfg_node
 */
cfg_node * get_cfg_node_after_ralated_loop(const super_block *spath,
		bit_set *path_related, const super_block_cfg *scfg) {
	assert(NULL != spath);
	assert(NULL != path_related);
	/**
	 * 1.get last related loop
	 */
	super_block *result_sb = NULL;
	cfg_node_list_iter cnli(spath->nodes());
	while (!cnli.is_empty()) {
		cfg_node *sn = cnli.step();
		super_block *sb = scfg->in_which_super_block(sn);
		if (super_block::LOOP == sb->knd()) {
			bit_set_iter iter(path_related);
			while (!iter.is_empty()) {
				int n = iter.step();
				machine_instr *mi =
						(machine_instr *) the_reach->catalog()->lookup(n);
				super_block *tmp_sb = scfg->in_which_super_block(
						mi->parent()->list_e());
				if (sb == tmp_sb) {
					result_sb = sb; //不用break, 这样记录的是最后一次相等的super_block;
				}
			}
		}
	}

	if (NULL == result_sb) {
		return NULL;
	}
	/**
	 * 2.get super block after the last related loop
	 */
	cnli.reset(spath->nodes());
	bool contains = false;
	while (!cnli.is_empty()) {
		cfg_node *sn = cnli.step();
		if (result_sb->contains(sn)) {
			contains = true;
		}
		if (contains && !result_sb->contains(sn)) {
			return sn;
		}
	}

	assert(false);
	//如果执行到此处，则不合理
	return NULL; //为消除warning, 特添加
}


/**
 * @descript - 获取src_cnl中以start_cn开始的子cfg_node_list
 * @param[in] src_cnl
 * @param[in] start_cn
 * @others  - if cn is not contains in src_cnl, return copy(src_cnl)
 */
cfg_node_list *sub_cnl(const cfg_node_list *src_cnl, const cfg_node *start_cn){
	assert(NULL != src_cnl);

	cfg_node_list *ret_cnl = new cfg_node_list;
	if (NULL != start_cn && NULL != src_cnl->lookup(start_cn)) {
		bool is_begin = false;
		cfg_node_list_iter cnli(src_cnl);
		while (!cnli.is_empty()) {
			cfg_node *step_cn = cnli.step();
			if (step_cn == start_cn) {
				is_begin = true;
			}
			if (is_begin) {
				ret_cnl->append(step_cn);
			}
		}
	} else {
		cfg_node_list_iter cnli(src_cnl);
		while (!cnli.is_empty()) {
			cfg_node *step_cn = cnli.step();
			ret_cnl->append(step_cn);
		}
	}

	return ret_cnl;
}


/**
 * @descript - 获取src_cnl中end_cn之前的子cfg_node_list
 * @param[in] src_cnl
 * @param[in] end_cn
 * @others  - if cn is not contains in src_cnl, return copy(src_cnl)
 */
cfg_node_list *sub_cnl_front(const cfg_node_list *src_cnl, const cfg_node *end_cn){
	assert(NULL != src_cnl);

	cfg_node_list *ret_cnl = new cfg_node_list;
	if (NULL != end_cn && NULL != src_cnl->lookup(end_cn)) {
		bool is_begin = true;
		cfg_node_list_iter cnli(src_cnl);
		while (!cnli.is_empty()) {
			cfg_node *step_cn = cnli.step();
			if (step_cn == end_cn) {
				is_begin = false;
			}
			if (is_begin) {
				ret_cnl->append(step_cn);
			}
		}
	} else {
		cfg_node_list_iter cnli(src_cnl);
		while (!cnli.is_empty()) {
			cfg_node *step_cn = cnli.step();
			ret_cnl->append(step_cn);
		}
	}

	return ret_cnl;
}

/**
 * @descript - 根据tnle分割src_cnl，取后部分
 * @param[in] src_cnl
 * @param[in] te
 * @others  - assert(), src_cnl contains te
 *            循环处理时调用
 */
cfg_node_list *sub_cnl_after_tnle(const cfg_node_list *src_cnl, tnle *te) {
	assert(NULL != src_cnl);
	assert(NULL != te);

	cfg_node *start_cn = NULL;
	cfg_node_list_iter cnli(src_cnl);
	while (!cnli.is_empty()) {
		cfg_node *step_cn = cnli.step();

		if (step_cn->is_block()) {
			if (((cfg_block*) step_cn)->contains(te)) {
				start_cn = step_cn;
				break;
			}
		}
	}

	assert(NULL != start_cn);
	return sub_cnl(src_cnl, start_cn);

}





//--------------------------------for test -----------------
/**
 * (num)_size, (num)_size,
 */
void test_print_num_vec(FILE *fp, const std::vector<int> num_vec, da_cfg *cfg){
	int sz = num_vec.size();
	if(num_vec.empty()){
		return;
	}

	int size = 0;
	cfg_node *cn = NULL;

	fprintf(fp, "num_vec list: (num)size\n");
	std::vector<int>::const_iterator it = num_vec.begin();
	for( ; it != num_vec.end() ;++it){
		cn = cfg->node(*it);
		size = cfg_node_static_instr_size(cn);
		fprintf(fp, "[%d]_%d, ", *it, size);
		fflush(fp);

		if(it != num_vec.begin()){// num_vec第一个节点可能是loop_begin
			assert(!cfg->is_loop_begin(cn));
		}

	}
	fprintf(fp, "\n");
}


/**
 * [1,2,3], (4),  [5, 6, 7],  (8)
 * 注意：循环处理过程中scfg还没有构造出来，此函数不能在循环处理中调用
 */
void test_print_node_list(FILE *fp, const util::node_list *head, da_cfg *cfg){
	assert(NULL != head);

	fprintf(fp, "node_list: (num_list)_size\n");
	util::node_list_iter it(head);
	while(!it.is_empty()){
		util::node *nd = it.step();
		nd->print(fp);
		fprintf(fp, "_%d, ", get_node_group_size(nd->getNode_group(), cfg));
	}
	fprintf(fp, "\n");
}



/**
 * [1,2,3]:(0x0), [5, 6, 7]_(0xfffff)
 */
void test_print_node_with_sp(FILE *fp, const util::node_list  *head, const super_block_cfg *scfg){
	assert(NULL != head);

	fprintf(fp, "format: [2, 3]_0xfff \n");
	util::node_list_iter it(head);
	while(!it.is_empty()){
		util::node *nd = it.step();
		nd->print(fp);
		fprintf(fp, "_%p, ", nd->getSpawn_pos(), scfg);
	}
	fprintf(fp, "\n");
	fflush(fp);
}


void test_print_vec(FILE *fp, const std::vector<double> vec)
{
	for (int i = 0; i < vec.size(); ++i) {
		fprintf(fp, "%f,    ", vec[i]);
	}
	fprintf(fp, "\n");
	fflush(fp);
}

