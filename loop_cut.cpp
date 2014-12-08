/*
 * loop_cut.cpp
 *
 *  Created on: 2012-3-16
 *      Author: harry
 *      注意：再循环划分过程中，super_block,super_block_cfg,还没有构造出来，不能使用
 *           只能使用da_cfg
 */


#include "loop_cut.h"
#include "common.h"
#include "misc.h"
#include "spmt_instr.h"
#include "spawn_pos_trace.h"
#include "spmt_fun.h"
#include "da_cfg.h"
#include "min_cut.h"
#include "min_cut_fun.h"
#include "helper.h"
#include "threshold.h"

#include <cstdio>
#include <stack>
#include <cassert>
#include <algorithm>

//extern FILE *loop_fp;  /*获取loop的中间数据*/
extern std::vector<loop_block *>*loops; /*vector which contains loop super block */

/*
 * 0.construct loop region
 *1.partition current loop->partition
 *2.if loop is big enough->partition likely_path
 *3.if has innerLoops, process loops.
 */
loop_block *partition_loop(cfg_node * loop_entry) {
	loop_block *loop = construct_loop(loop_entry);
	loop->insert_loop_instrs();
	//if (verbose)
		loop->print();

	//1.partition current loop->partition
	partition_curr_loop(loop);

	//2.递归激发内嵌的循环
	cfg_node *loop_end = loop->end_node();
	const int loop_end_depth = the_cfg->loop_depth(loop_end);
	cfg_node_list_iter cnli(loop->nodes());
	while (!cnli.is_empty()) {
		cfg_node *step_cn = cnli.step();
		//处理内层循环
		if (the_cfg->loop_depth(step_cn) == loop_end_depth + 1
				&& the_cfg->is_loop_begin(step_cn)) {
			partition_loop(step_cn);
			loop->set_block_num(step_cn->number());
		}
	}

	return loop;
}

/*
 *construct_loop() --- 构建循环区域
 *蕴含一个经典算法，可以参考龙书
 */
loop_block *construct_loop(cfg_node * loop_entry) {
	bit_set the_loop(0, the_cfg->num_nodes());
	std::stack<cfg_node *> node_stack;

	cfg_node *loop_end;
	cfg_node_list_iter pred_iter(loop_entry->preds());

	//look up the end node of loop region
	while (!pred_iter.is_empty()) {
		cfg_node *pred_cn = pred_iter.step();
		if (the_cfg->dominates(loop_entry, pred_cn)
				&& the_cfg->loop_depth(pred_cn)
						== the_cfg->loop_depth(loop_entry)) {
			loop_end = pred_cn;
			//需要处理continue等有两个回边的情况，
			//不能加上break退出，分析em3d的make_neighbors发现有两个回边，需要选范围大的
		}
	}

	node_stack.push(loop_end);
	the_loop.add(loop_entry->number());

	//record the cfg node of loop region
	while (!node_stack.empty()) {
		cfg_node *top = node_stack.top();
		node_stack.pop();

		int num = top->number();
		if (!the_loop.contains(num)) {
			the_loop.add(num);
			cfg_node_list_iter top_iter(top->preds());
			while (!top_iter.is_empty()) {
				node_stack.push(top_iter.step());
			}
		}
	}

	//allocate loop block
	loop_block *loop = new loop_block();
	loop->set_entry_node(loop_entry);
	loop->set_end_node(loop_end);

	bit_set_iter loop_iter(&the_loop);
	while (!loop_iter.is_empty()) {
		int num = loop_iter.step();
		cfg_node *node = the_cfg->node(num);
		loop->add_block(node);
		if (the_cfg->is_loop_exit(node)) {
			loop->add_exit_node(node);
		}
	}

	return loop;
}


/**
 * 做两类划分:
 * 	第一类划分:迭代间的划分，避免无限激发问题
 * 	第二类划分:循环内部的划分，
 * 	直到两次划分完成了之后才设置所有的sp点
 */
void partition_curr_loop(loop_block *loop)
{
	assert(NULL != loop);
	/**
	 * 1.获取循环的likely_path
	 */
	super_block *loop_likely_path = loop->most_likely_path();
	printf("loop_likely_path:\n");
	loop_likely_path->print(stdout);
	printf("\n");

	/**
	 *  第02步:迭代间的划分，避免无限激发问题
	 */
	tnle *cqip_pos = NULL;
	tnle *spawn_pos = NULL;
	bit_set pslice_dept;//用来记录相关点
	pslice_dept.clear();
	bool bResult = get_best_loop_cqip(loop, cqip_pos, spawn_pos, &pslice_dept);
	if(bResult){
		//划分成功
		cfg_block *spawn_block = loop_likely_path->in_which_block(spawn_pos);
		cfg_block *cqip_block =  loop_likely_path->in_which_block(cqip_pos);
		printf("循环划分成功!\n");
		printf("sp at block_%d:\n", spawn_block->number());
		((machine_instr*)((tree_instr *)(spawn_pos->contents))->instr())->print(stdout);
		printf("\n");
		printf("cqip at block_%d:\n", cqip_block->number());
		((machine_instr*)((tree_instr *)(cqip_pos->contents))->instr())->print(stdout);
		printf("\n");
		printf("pslice_dept is:\n", cqip_block->number());
		pslice_dept.print(stdout);
		printf("\n");

		loop->set_cqip_pos(cqip_pos);
		loop->set_spawned_pos(spawn_pos);
		loop->set_cqip_block(cqip_block);
		loop->set_pslice_dept(&pslice_dept);
		construct_loop_thread(loop);
	}

}


/**
 * 根据loop_likely_path上的内部循环，将loop_likely_path_vec分裂成多个natural_groups
 */
util::node_list *split_loop_likely_path_by_loop(const std::vector<int> &loop_likely_path_vec) {
	util::node_list *head = new util::node_list();
	/**
	 * (1).按照循环分割成大节点组——
	 *     大节点组就是仅仅根据循环分割的
	 *     是之后处理划分的基本单位
	 */
	util::node *pNode = NULL;
	std::vector<int> vec;
	int preIndex = 0;
	/**
	 * 从i=1开始处理，因为loop_likely_path的第一个节点是外层的loop_begin
	 */
	int i = 1;
	for (; i < loop_likely_path_vec.size(); ++i) {
		if (the_cfg->is_loop_begin(loop_likely_path_vec[i])) {
			if (i > preIndex) {
				vec = sub_num_vec(loop_likely_path_vec, preIndex, i);
				pNode = new util::node(vec);
				pNode->setKind(util::node::NODE_GROUP);
				head->append(pNode);
				vec.clear();
				pNode = NULL;
			}

			//对循环归结处理,然后修改i值和preIndex值
			loop_block *loop = construct_loop(the_cfg->node(loop_likely_path_vec[i]));
			//此处最好assert loop_likely_path_vec contains loop
			cfg_node *succ_node = loop->find_succ_node(the_cfg);
			const int succ_index = find_index(loop_likely_path_vec, succ_node->number());
			i = succ_index - 1;
			preIndex = succ_index;
		}
	}

	if (preIndex < loop_likely_path_vec.size()) {
		vec = sub_num_vec(loop_likely_path_vec, preIndex, i);
		pNode = new util::node(vec);
		pNode->setKind(util::node::NODE_GROUP);
		head->append(pNode);
		vec.clear();
	}
	return head;
}





/**
 * 对已经获得sp和cqip的loop进行构造线程工作
 */
void construct_loop_thread(loop_block *loop){
	assert(NULL != loop->get_cqip_pos());
	assert(NULL != loop->get_spawned_pos());

	cfg_block *cqip_block = loop->get_cqip_block();
	super_block *loop_likely_path = loop->most_likely_path();
	tnle *spawn_pos = loop->get_spawned_pos();
	tnle *old_cqip_pos = loop->get_cqip_pos();
	//insert cqip point for loop region
	//tnle *cqip_pos = insert_cqip_instr_for_loop(cqip_block);
	//将cqip_pos将原始指令换成现在的前面一条指令
	tnle *new_cqip_pos = insert_cqip_instr_for_loop(cqip_block, old_cqip_pos);
	label_sym *cqip_pos_num = peek_cqip_pos(new_cqip_pos);


	//find live-ins and construct pre-compute slice
	/*bit_set liveins;
	//the_cfg->find_liveins_of_loop(loop, the_bit_mgr, &liveins);
	the_cfg->find_liveins(loop_likely_path, the_bit_mgr, &liveins);
	printf("in construct_loop_thread, livein is:\n");
	liveins.print(stdout);
	printf("\n");
	bit_set *ins_dep = new bit_set(0, the_reach->catalog()->num_defs());
	search_pslice_instrs(&liveins, loop_likely_path, spawn_pos, ins_dep);*/
	bit_set *ins_dep = new bit_set(0, the_reach->catalog()->num_defs());
	*ins_dep = *(loop->get_pslice_dept());
	tree_node_list *pslice = new tree_node_list;
	construct_pslice(ins_dep, cqip_pos_num, pslice);

	printf("pslice is:\n");
	pslice->print(stdout);
	printf("\n");
	cfg_block *spawn_block = loop_likely_path->in_which_block(spawn_pos);

	//set spawn information for the loop region
	loop->find_succ_node(the_cfg);
	//printf("loop_succ_node:");

	loop->set_cqip_pos(new_cqip_pos);
	loop->set_cqip_block(cqip_block);
	loop->set_spawn_info(spawn_pos, spawn_block, pslice, cqip_pos_num);

	loops->push_back(loop);

	delete ins_dep;
}

/**
 * find best cqip_pos and spawn_pos for loop
 * @param[in]  loop
 * @param[out] cqip_pos
 * @param[out] spawn_pos , NULL if not found
 * 注意：循环划分中不能使用the_scfg，因为the_scfg在循环划分归结后才构造完成
 */
bool get_best_loop_cqip(loop_block *loop, tnle *&cqip_pos, tnle *&spawn_pos, bit_set *pslice_dept) {
	assert(NULL != loop);
	bool bRet = false;
	//判断最后两条指令是条件跳转指令
	/*cfg_block *end_blk = (cfg_block*) loop->end_node();
	if(!end_blk->ends_in_cbr()){
		return bRet;
	}*/

	/**
	 * 经过多方测试，发现有些循环会导致最后一个结点只含有一个条件跳转指令，
	 * 就是条件跳转的两条指令被分割到了两个基本块中，
	 * 此时，我们需要从直接前驱中寻找可供作为替代的cqip节点
	 * 例如：for(i=1; i && i< 10; ++i); 会导致这种情况
	 * 又如：mst的HashLookup和power的optimize_node的B77
	 */
	/*int static_size = cfg_node_static_instr_size(end_blk);
	if(1 == static_size){
		return bRet;
	}*/

	//-----------------新方案-----------------
	if(loop->has_inner_loop()){
		bRet = get_best_cqip_for_loop_with_innerloop(loop, cqip_pos, spawn_pos, pslice_dept);
	}else{
		bRet = get_best_cqip_for_loop_without_innerloop(loop, cqip_pos, spawn_pos, pslice_dept);
	}

	return bRet;

	/*instruction *in = ((tree_instr*) end_blk->last_non_cti()->contents)->instr();
	printf("end_block is:\n");
	end_blk->print_with_instrs(stdout);
	fflush(stdout);

	printf("cqip pos is:\n");
	((machine_instr *)in)->print(stdout);
	fflush(stdout);
	assert(is_cmp_op(in));


	cqip_pos = in->parent()->list_e();*/

	/**
	 * 1.在likely_path上选取最好的cqip点, cqip点尽可能靠近loop_end的条件跳转指令
	 */
	//(0). 在loop_likely_path上获取
	//     spawn_path_list(计算激发距离)和nloop_spawn_path(用来寻找sp点)
	//super_block *loop_likely_path = loop->most_likely_path();

	//spawn_path用来计算激发距离
	//super_block *spawn_path = loop_likely_path;

	//用来寻找sp点
	//需要根据related_loop和related_call裁剪
	/*super_block *non_related_spawn_path = get_loop_effective_spawn_path(loop);
	printf("激发路径裁剪后为:\n");
	non_related_spawn_path->print(stdout);
	printf("\n");

	super_block *non_innerloop_cqip_path = get_non_innerloop_cqip_path(loop);

	//(2).获取数据流信息
	bit_set liveins;
	the_cfg->find_liveins(non_innerloop_cqip_path, the_bit_mgr, &liveins); //globe-var the_cfg , the_bit_mgr
	printf("in get_best_loop_cqip, livein is:\n");
	liveins.print(stdout);
	printf("\n");
	bit_set *path_related = new bit_set(0, the_reach->catalog()->num_defs());
	path_related->clear();
	search_related_point(&liveins, non_related_spawn_path, path_related);
	printf("path_related is:\n");
	path_related->print(stdout);
	printf("\n");*/

	//(3).根据数据流信息(related_call和related_loop)裁剪激发路径

	//(3).我们就将那个地方设置为cqip点，针对指令做def和use分析太麻烦了,需要自己实现库的功能
	//直接寻找sp点
	/*int spawn_distance = 0;
	tnle *t_e = NULL;
	bit_set *ins_dep = new bit_set(0, the_reach->catalog()->num_defs());
	int total_count = non_related_spawn_path->static_size();

	for (int index = 0; index < total_count-2; ++index) {*/
		/**
		 * 从第一条指令开始，看是否满足一下条件
		 * 1.dependence_threshold
		 *
		 */
		/*machine_instr *spawn_pos_instr = non_related_spawn_path->instr_access(index);
		assert(NULL != spawn_pos_instr);
		t_e = spawn_pos_instr->parent()->list_e();

		*ins_dep = *path_related;
		pruning_instrs_before_spawn(ins_dep, non_related_spawn_path, t_e);

		printf("ins_dep is:\n");
		ins_dep->print(stdout);
		printf("\n");*/
		/*int cnt1 = path_related->count(); // for test
		int cnt2 = ins_dep->count(); // for test*/

		/*spawn_distance = compute_spawning_distance_from_sp(non_related_spawn_path, spawn_pos_instr) - 2;

		if (ins_dep->count() < (int) threshold::nloop_dependence_threshold
				&& spawn_distance > threshold::nloop_spawning_distance_lower) {

			printf("sp获取成功， %d条pslice语句如下：\n", ins_dep->count());
			bit_set_iter iter(ins_dep);
			while(!iter.is_empty()){
				int n = iter.step();
				machine_instr *m_instr = (machine_instr *) the_reach->catalog()->lookup(n);
				m_instr->print(stdout);
				fflush(stdout);
			}

			spawn_pos = t_e;
			bRet = true;
			break;
		}
	}


	delete path_related;
	delete ins_dep;
	return bRet;*/
}


/**
 * 为具有innerLoop的循环寻找最优cqip,
 * 由于具有innerLoop,我们可以将其根据innerLoop将其隔断
 */
bool get_best_cqip_for_loop_with_innerloop(loop_block *loop, tnle *&cqip_pos, tnle *&spawn_pos, bit_set *pslice_dept){
	printf("in get_best_cqip_for_loop_with_innerloop...\n");

	super_block *nxt_cqip_path = get_non_innerloop_cqip_path(loop);//根据innerLoop裁剪
	super_block *biggest_spawn_path =  prunged_loop_path_by_inner_loop(loop);//根据innerLoop裁剪
	super_block *effect_spawn_path = prunged_loop_path_by_pos_trace(biggest_spawn_path, nxt_cqip_path);
	printf("nxt_cqip_path:\n");
	nxt_cqip_path->print(stdout);
	printf("biggest_spawn_path:\n");
	biggest_spawn_path->print(stdout);
	printf("effect_spawn_path:\n");
	effect_spawn_path->print(stdout);

	/**
	 *为effect_spawn_path ————> nxt_cqip_path 寻找最优cqip点，cqip点和spos都从effect_spawn_path中寻找
	 */
	//迭代取cqip点计算
	super_block *temp_sb = new super_block();
	temp_sb->add_blocks(effect_spawn_path->nodes());
	temp_sb->add_blocks(nxt_cqip_path->nodes());
	int total_count = effect_spawn_path->static_size();

	int max_value = -1;
	//[3, 4], (5), [6, 7] ——> [6, 7, 3, 4]则至少从index=3开始，否则没地方放sp点
	for(int cqip_index = 3; cqip_index < total_count-2; ++cqip_index){
		//
		machine_instr *cqip_instr = effect_spawn_path->instr_access(cqip_index);
		printf("it_cqip_instr:\n");
		cqip_instr->print(stdout);
		tnle *it_cqip_pos = cqip_instr->parent()->list_e();
		bit_set it_pslice_dept;
		it_pslice_dept.clear();
		//为其寻找最佳sp点
		//重新计算live_in
		bit_set liveins;
		tnle *it_sp_pos = NULL;
		the_cfg->find_liveins_after_pos(temp_sb, cqip_instr, the_bit_mgr, &liveins);
		int evalue = get_best_sp_at_given_cqip(effect_spawn_path, cqip_instr,&liveins, it_cqip_pos, it_sp_pos, &it_pslice_dept);

		if(evalue >= 2 && evalue > max_value){
			max_value = evalue;
			cqip_pos = it_cqip_pos;
			spawn_pos = it_sp_pos;
		}
	}

	delete temp_sb;

	if(max_value > 0){
		return true;
	}else{
		return false;
	}
}


bool get_best_cqip_for_loop_without_innerloop(loop_block *loop, tnle *&cqip_pos,
		tnle *&spawn_pos, bit_set *pslice_dept) {

	super_block *loop_path = loop->most_likely_path();

	/**
	 *为loop_path 寻找最优cqip点，cqip点和spos都从loop_path中寻找
	 */
	//迭代取cqip点计算
	int total_count = loop_path->static_size();
	cfg_block *cb = (cfg_block *)loop->end_node();
	tnle *te = cb->first_active_op();
	machine_instr * begin_mi =(machine_instr*)((tree_instr *)(te->contents))->instr();

	int begin_index = loop_path->instr_count(begin_mi);

	int max_value = -1;

	for (int cqip_index = begin_index; cqip_index < total_count - 1; ++cqip_index) {
		//
		machine_instr *cqip_instr = loop_path->instr_access(cqip_index);
		printf("it_cqip_instr:\n");
		cqip_instr->print(stdout);
		tnle *it_cqip_pos = cqip_instr->parent()->list_e();
		bit_set it_pslice_dept;
		it_pslice_dept.clear();
		//为其寻找最佳sp点
		//重新计算live_in
		bit_set liveins;
		tnle *it_sp_pos = NULL;
		the_cfg->find_loop_liveins_at_cqip_pos(loop_path, cqip_instr, the_bit_mgr,
				&liveins);
		bit_set instr_dept;
		int evalue = get_best_sp_at_given_cqip(loop_path, cqip_instr, &liveins, it_cqip_pos, it_sp_pos, &it_pslice_dept);

		if (evalue >= 2 && evalue >= max_value) {
			max_value = evalue;
			cqip_pos = it_cqip_pos;
			spawn_pos = it_sp_pos;
			*pslice_dept = it_pslice_dept;
		}
	}


	if (max_value > 0) {
		printf("最佳划分结果：\n");
		printf("cqip_pos is:\n");
		((machine_instr*)((tree_instr *)(cqip_pos->contents))->instr())->print(stdout);
		printf("\nsp_pos is:\n");
		((machine_instr*)((tree_instr *)(spawn_pos->contents))->instr())->print(stdout);
		printf("\n");
		printf("\npslice is:\n");
		pslice_dept->print(stdout);
		printf("\n");

		return true;
	} else {
		return false;
	}

}



/**
 * @param[in]  spawn_sb
 * @param[in]  cqip_instr
 * @param[in]  live_ins
 * @param[in] cqip_pos
 * @param[out] spawn_pos
 * @param[out] pslice_dept ---依赖指令，最后直接作为pslice
 */
int get_best_sp_at_given_cqip(super_block *spawn_sb, machine_instr *cqip_instr,
		bit_set *live_ins, tnle *cqip_pos, tnle *&spawn_pos, bit_set *pslice_dept) {
	int iRet = -1;
	bit_set *path_related = new bit_set(0, the_reach->catalog()->num_defs());
	path_related->clear();
	search_related_point_before_instr(live_ins, spawn_sb, cqip_instr,
			path_related);

	//我们选择评价值最大的返回
	tnle *iter_spawn_pos = NULL;
	int spawn_distance = 0;
	bit_set *ins_dep = new bit_set(0, the_reach->catalog()->num_defs());
	int total_count = spawn_sb->instr_count(cqip_instr);  //此处不能+1，因为cqip_instr不属于spawn部分

	for (int index = 0; index < total_count - 2; ++index) {

		machine_instr *spawn_pos_instr = spawn_sb->instr_access(index);
		if((int)spawn_pos_instr->opcode() == mo_mfhi || (int)spawn_pos_instr->opcode() == mo_mflo){
			continue;
		}
		assert(NULL != spawn_pos_instr);
		printf("spawn_pos_instr:\n");
		spawn_pos_instr->print(stdout);
		printf("\n");
		iter_spawn_pos = spawn_pos_instr->parent()->list_e();

		*ins_dep = *path_related;
		pruning_instrs_before_spawn(ins_dep, spawn_sb, iter_spawn_pos);

		printf("ins_dep is:\n");
		ins_dep->print(stdout);
		printf("\n");
		int cnt1 = path_related->count(); // for test
		int cnt2 = ins_dep->count(); // for test

		spawn_distance = compute_spawning_distance_between_sp_cqip(spawn_sb, spawn_pos_instr, cqip_instr);
		int dep_cnt = ins_dep->count();
		if (dep_cnt < (int) threshold::nloop_dependence_threshold
				&& spawn_distance < threshold::nloop_spawning_distance_upper
				&& spawn_distance - dep_cnt >= 2) {
			int iter_value = spawn_distance - dep_cnt;
			printf("sp获取成功， %d条pslice语句如下：\n", ins_dep->count());
			bit_set_iter iter(ins_dep);
			while (!iter.is_empty()) {
				int n = iter.step();
				machine_instr *m_instr =
						(machine_instr *) the_reach->catalog()->lookup(n);
				m_instr->print(stdout);
				fflush(stdout);
			}

			if (iter_value > iRet) {
				iRet = iter_value;
				spawn_pos = iter_spawn_pos;
				*pslice_dept = *ins_dep;
			}
		}
	}

	delete path_related;
	delete ins_dep;
	return iRet;
}







/**
 * @description - 用于处理hasInnerLoop的循环，
 *                输入的是根据innerLoop裁剪的最前面的一段nxt_cqip_path和最后一段biggest_spawn_path
 *                根据related_call裁剪biggest_spawn_path
 */
super_block *prunged_loop_path_by_pos_trace(super_block *biggest_spawn_path, super_block *nxt_cqip_path) {
	/**
	 * 1.根据spawn_pos裁掉related_call前面的
	 */
	super_block *target = NULL;

	bit_set liveins;
	the_cfg->find_liveins(nxt_cqip_path, the_bit_mgr, &liveins); //globe-var the_cfg , the_bit_mgr

	//先由spawn_pos_trace确定
	spawn_pos_trace *pos_trace = new spawn_pos_trace();
	bit_set *spawn_related = new bit_set(0, the_reach->catalog()->num_defs());
	spawn_related->clear();

	pos_trace->construct_loop_spawn_pos_trace(biggest_spawn_path);
	search_related_point(&liveins, pos_trace, spawn_related);
	pos_trace->analyze_related_cals(the_reach, spawn_related);

	std::vector<machine_instr *> *cal_instrs = pos_trace->get_related_cals();
	if (!cal_instrs->empty()) { //确定最后一次的call指令，因为break_at_call，所以直接从下一个节点开始
		std::vector<machine_instr *>::reverse_iterator riter = cal_instrs->rbegin();
		machine_instr *last_cal_instr = *riter;
		tree_instr *ti = last_cal_instr->parent();
		tnle * t_e = ti->list_e();
		cfg_node_list *cnl= sub_cnl_after_tnle(biggest_spawn_path->nodes(), t_e);
		target = new super_block();
		target->add_blocks(cnl);
		delete cnl;
	} else {
		target = biggest_spawn_path;
	}

	delete pos_trace;
	delete spawn_related;
	return target;
}


/**
 * @description - 通过对loop_likely_path
 *                根据related_loop裁剪
 * @others - 程序中假定每一个循环中，节点编号loop_end最大， loop_begin最小
 */
super_block *prunged_loop_path_by_inner_loop(loop_block *loop){
	super_block *loop_likely_path = loop->most_likely_path();

	cfg_node *loop_end = loop->end_node();
	const int loop_end_depth = the_cfg->loop_depth(loop_end);
	cfg_node_list_iter cnli(loop_likely_path->nodes());

	cfg_node *start_cn = NULL;
	while (!cnli.is_empty()) {
		cfg_node *step_cn = cnli.step();
		//处理内层循环
		if (the_cfg->loop_depth(step_cn) == loop_end_depth + 1
				&& the_cfg->is_loop_end(step_cn)) {
			start_cn = step_cn->fall_succ();
		}

	}


	if (start_cn != NULL) {
		//获取target所在的循环的loop_end(loop_depth都相等)，取其下一个cfg_node作为start

		cfg_node_list *cnl = sub_cnl(loop_likely_path->nodes(), start_cn);
		super_block *ret_sb = new super_block();
		ret_sb->add_blocks(cnl);
		return ret_sb;
	} else {
		return loop_likely_path;
	}
}



/**
 * 返回loop的most_likely_path上 从开始节点到第一个innerLoop之前的节点
 */
super_block *get_non_innerloop_cqip_path(loop_block *loop) {
	super_block *ret_sb = NULL;

	super_block *loop_likely_path = loop->most_likely_path();

	cfg_node *loop_end = loop->end_node();
	const int loop_end_depth = the_cfg->loop_depth(loop_end);
	cfg_node_list_iter cnli(loop_likely_path->nodes());

	cfg_node *target = NULL;
	while (!cnli.is_empty()) {
		cfg_node *step_cn = cnli.step();
		//处理内层循环
		if (the_cfg->loop_depth(step_cn) == loop_end_depth + 1
				&& the_cfg->is_loop_begin(step_cn)) {
			target = step_cn;
			break;
		}
	}

	if (target != NULL) {
		cfg_node_list *cnl = sub_cnl_front(loop_likely_path->nodes(), target);
		ret_sb = new super_block();
		ret_sb->add_blocks(cnl);
		return ret_sb;
	} else {
		return loop_likely_path;
	}
}



//--------------------------test------------------------
void test_print_loop_end(FILE *fp, cfg_block *blk)
{
	fprintf(fp, "-----------Block_%d-----------\n", blk->number());
	blk->print_with_instrs(fp);
	fprintf(fp, "\n\n");
	fflush(fp);
}




