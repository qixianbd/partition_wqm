/*
 * min_cut.cpp
 *
 *Implement the algorithm to partition non-loop region
 *
 *  Created on: 2011-12-22
 *      Author: harry
 */

#include "min_cut.h"
#include "min_cut_fun.h"
#include "spmt_fun.h"
#include "super_block_path.h"
#include "super_block.h"
#include "spawn_pos_trace.h"
#include "da_cfg.h"
#include "threshold.h"
#include "misc.h"
#include "node.h"
#include "helper.h"

#include <vector>
#include <iostream>

extern std::vector<thread *>*threads;



void non_loop_partition(super_block_cfg *scfg)
{
	extern std::vector<loop_block *>*loops; /*vector which contains loop super block */

	//1. 对于非循环区域的第一遍划分
	non_loop_partition_first_pass(scfg);

	//2. 对于非循环区域的第二遍和更多遍划分
	//2.1 获取之前划分的所有sp_block 和 cqip_block.
	std::vector<thread *>::iterator iter;
	for (iter = threads->begin(); iter != threads->end(); iter++) {
		thread *curr_thread = *iter;
		int sp_block_num = curr_thread->get_spawned_block()->number();
		int cqip_block_num = curr_thread->get_cqip_block()->number();
		scfg->add_sp_block(sp_block_num);
		scfg->add_cqip_block(cqip_block_num);
	}

	std::vector<loop_block *>::iterator loop_iter = loops->begin();
	for(; loop_iter != loops->end(); loop_iter++){
		loop_block *curr_loop_block = *loop_iter;
		int sp_block_num = curr_loop_block->get_spawned_block()->number();
		int cqip_block_num = curr_loop_block->get_cqip_block()->number();
		scfg->add_sp_block(sp_block_num);
		scfg->add_cqip_block(cqip_block_num);
	}

	fprintf(stderr, "sp block list  and cqip block list.\n");
	scfg->print_sp_and_cqip_block_list(stderr);
	fprintf(stderr, "The content of sp block attribute list as follows:\n");
	scfg->fill_sp_attribute_map();
	scfg->print_sp_attribute_map(stderr);

	//const std::vector<int> block_num_vec = get_block_num_vec_from_scfg(scfg);

	/**
	 * (0).init-构造得到一个节点组
	 */
	//const std::vector<int> num_vec = get_num_vec_from_scfg(scfg);    //选取最可能路径上的基本块.

	int all_partition_flag = 0;

	while(! all_partition_flag){
		int with_new_flag = 0;
		std::vector<int> num_vec_path = get_possible_path_num_vec_from_scfg(scfg, with_new_flag);		//???? need to be completed.
										/*
										 * 目前get_possible_path_num_vec_from_scfg返回的路径条数为total_count = max{m1, m2, m3, mi...}, 其中mi为sp block所在的路径数
										 * 我们想让其返回的路径条数为total_count = m1*m2*m3*m4*mi....
										 */
		if(num_vec_path.empty() || with_new_flag == 0){
			all_partition_flag = 1;
			break;
		}

		/**
		 * (1).按照循环自然分割——节点组分裂,
		 *     natual_node_groups表示由自然循环分割的节点组链表(指向包含一连串节点组的链表容器)
		 */
		util::node_list *natual_node_groups = split_node_group_by_loop(num_vec_path, scfg);

		/**
		 *  (2).节点组分裂过程
		 *      遍历node_list过程，对每个节点组根据粒度进行划分
		 *      中间结果统计见 麦库
		 */
	    split_nloop_region(natual_node_groups, the_cfg);

		/**
		 * (3).计算各个节点组的sp点
		 * 为什么不能一边划分，一边计算sp点？
		 * 因为我们采用迭代的划分过程，下一迭代会改变上一次划分的group的最优sp点位置。
		 * 所以，必须等划分确定后，才计算sp点
		 */
	    destruct_node_list(natual_node_groups);
	}

}
//static bool min_cut_verbos = true;

/**
 * @description -(0).初始化-得到一个节点组
 *               (1).按照循环自然分割——节点分裂
 *               (2).节点组分裂
 *               (3).重新计算sp点
 * @param scfg - the super_block_cfg of the procedure,  readonly
 * @return node_list 单链表结构，每一个node表示一个节点组
 */
void non_loop_partition_first_pass(super_block_cfg *scfg)
{
	/**
	 * (0).init-构造得到一个节点组
	 */
	const std::vector<int> num_vec = get_num_vec_from_scfg(scfg);    //选取最可能路径上的基本块.
	//test_print_num_vec(stdout, num_vec, scfg);
#define KM_DEBUG
#ifdef KM_DEBUG
	int path_size = num_vec.size();
	for(int i = 0; i < path_size; i++){
		fprintf(stderr, "%d \t",num_vec[i]);
	}
	fprintf(stderr, "\n");
#endif

	/**
	 * (1).按照循环自然分割——节点组分裂,
	 *     natual_node_groups表示由自然循环分割的节点组链表(指向包含一连串节点组的链表容器)
	 */
	util::node_list *natual_node_groups = split_node_group_by_loop(num_vec, scfg);

	/**
	 *  (2).节点组分裂过程
	 *      遍历node_list过程，对每个节点组根据粒度进行划分
	 *      中间结果统计见 麦库
	 */
    split_nloop_region(natual_node_groups, the_cfg);

	/**
	 * (3).计算各个节点组的sp点
	 * 为什么不能一边划分，一边计算sp点？
	 * 因为我们采用迭代的划分过程，下一迭代会改变上一次划分的group的最优sp点位置。
	 * 所以，必须等划分确定后，才计算sp点
	 */
    destruct_node_list(natual_node_groups);
}


/**
 * +[测试通过]
 *
 * 根据循环，将num_vec分裂成多个节点组，num_ve已经被处理过了(不包含entry和exit节点)
 * 初始使用，减小初始规模
 */
util::node_list *split_node_group_by_loop(const std::vector<int> &num_vec,
		super_block_cfg *scfg) {
	util::node_list *head = new util::node_list();
	/**
	 * (1).按照循环分割成大节点组——
	 *     大节点组就是仅仅根据循环分割的
	 *     是之后处理划分的基本单位
	 */
	util::node *pNode = NULL;
	super_block* sb = NULL;
	std::vector<int> vec;
	int preIndex = 0;
	int i = 0;

	for (; i < num_vec.size(); ++i) {
		sb = scfg->get_super_block_by_num(num_vec[i]);
		switch (sb->knd()) { //不含CFG_BEGIN 和 CFG_END
		case super_block::LOOP:
			//将[preIndex, i)组成一个线程组，将[i]组成一个线程组,append入head中
		{
			if (i > preIndex) {
				vec = sub_num_vec(num_vec, preIndex, i);
				pNode = new util::node(vec);
				pNode->setKind(util::node::NODE_GROUP);
				head->append(pNode);
				vec.clear();
				pNode = NULL;
			}
			preIndex = i + 1;
			break;
		}
		case super_block::BLOCK:
			break;
		default:
			assert(false);
			break;
		}
	}

	if (preIndex < num_vec.size()) {
		vec = sub_num_vec(num_vec, preIndex, i);
		pNode = new util::node(vec);
		pNode->setKind(util::node::NODE_GROUP);
		head->append(pNode);
		vec.clear();
	}
	return head;
}


/**
 * 节点组分裂过程
 * 以根据循环划分的大节点组为单位进行划分
 * 中间结果统计见 麦库
 */
void split_nloop_region(util::node_list *&natual_node_groups, da_cfg *cfg) {
	/**
	 * 为避免之后对循环激发的影响，非循环的sp点不跨越loop，这样激发路径也在本 大节点组中
	 * 这样cqip_vec和spawn_vec都在本大节点组中，所以说 划分以大节点组为单位
	 */
	util::node_list_iter nli(natual_node_groups);
	while (!nli.is_empty()) {
		util::node *pNode = nli.step();
		pNode->print(stdout);
		if (pNode->is_node_group()) {
			std::vector<int> natual_group = pNode->getNode_group();
			printf("natural_group is: ");
			test_print_num_vec(stdout, natual_group, cfg);   // for test*/
			partition_natual_node_group(natual_group, cfg);
		}
	}
}


util::node_list *split_natural_node_group(const std::vector<int> & natual_group, da_cfg * cfg)
{
    /**
	 * 迭代划分，将natual_group组装成node_group,每一次划分一分为二，则reset。
	 */
    util::node_list *head = new util::node_list();
    util::node *pNode = new util::node(natual_group);
    pNode->setKind(util::node::NODE_GROUP);
    head->append(pNode);
    util::node_list_iter nli(head);
    while (!nli.is_empty()) {
		pNode = nli.step();
		if (pNode->is_node_group()) {
			std::vector<int> group = pNode->getNode_group();
			int npos = get_opt_split_pos(group, natual_group, cfg);
			assert(npos == -1 || npos > 0);
			if (npos > 0) {
				//split and reconstruct node_list

				std::vector<int> vec1 = sub_num_vec(group, 0, npos);
				std::vector<int> vec2 = sub_num_vec(group, npos);
				util::node* pNode1 = new util::node(vec1);
				pNode1->setKind(util::node::NODE_GROUP);
				util::node* pNode2 = new util::node(vec2);
				pNode2->setKind(util::node::NODE_GROUP);

				util::node_list_e* cur_elem = nli.cur_elem();
				//以下改变node_list结构的操作已经破坏了iter
				head->insert_before(pNode1, cur_elem);
				head->insert_before(pNode2, cur_elem);
				head->remove(cur_elem);
				nli.reset(head);
			}
		}
	}
    return head;
}

/**
 * @description - 对natual_node_group进行穷举划分
 *   1.划分
 *   2.寻找sp点
 * @others 先整体划分完毕，再寻找sp点，因为后一步划分会影响前一步划分的数据依赖
 */
void partition_natual_node_group(const std::vector<int> &natual_group, da_cfg *cfg)
{
	//边界检测
	int node_group_size = get_node_group_size(natual_group, cfg);
	if (natual_group.size() < 2 || !threshold::is_big(node_group_size)) {
		return;
	}

	/**
	 * 迭代划分，将natual_group组装成node_group,每一次划分一分为二，则reset。
	 */

#define KM_DEBUG
#ifdef KM_DEBUG
	int size = 0;
	size = natual_group.size();
	for(int i = 0; i < size; i++){
		fprintf(stderr, "%d \t", natual_group[i]);
	}
	fprintf(stderr, "\n");
#endif

	//1. determine cqip_block.
	//下面这一步是将一个函数分为若干个 node_list, 其中每一个node_list对应一个线程. 相当于在这个过程中确定了cqip所在的block
	util::node_list *head = split_natural_node_group(natual_group, cfg);


#define KM_DEBUG
#ifdef KM_DEBUG
	util::node_list_iter nli(head);
	while(!nli.is_empty()){
		util::node *pNode = nli.step();
		pNode->print(stderr);
		fprintf(stderr, "\n**********\n");
	}
#endif
	/**
	 * 本节点组划分完毕，寻找其sp点
	 * 该函数确定sp_block.
	 */
	//02. determine sp_block
	 set_sp_for_all_node_group(head, natual_group, cfg);


	 //determine pslice segment.
	 construct_nloop_thread(head, the_scfg);

	 destruct_node_list(head);


}



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
int get_opt_split_pos(const std::vector<int> &num_vec, const std::vector<int> &natual_group, da_cfg *cfg)
{
	assert(!num_vec.empty());
	/**
	 * 约束条件如下：
	 *1.线程粒度
	 *2.opt_dep
	 */
	std::vector<double> edge_evalue_vec(num_vec.size(), -1.0);//不使用第一个值
	for (int i = 1; i < num_vec.size(); ++i) {
		evalue_at_given_pos(num_vec, i, edge_evalue_vec, natual_group, cfg);
	}

	//test_print_vec(stdout, edge_evalue_vec);

	/**
	 * 选择满足阈值spawning_distance_lower的最大评估值，将节点组一分为二
	 */
	int max_index = 0;
	double max_value = edge_evalue_vec[0];
	for (int i = 1; i < edge_evalue_vec.size(); ++i) {
		if (edge_evalue_vec[i] > max_value) {
			max_value = edge_evalue_vec[i];
			max_index = i;
		}
	}

	if (max_index > 0) {
		return max_index;
	} else {
		return -1;
	}
}



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
double evalue_at_given_pos(const std::vector<int> &num_vec, int npos, std::vector<double> &edge_evalue_vec, const std::vector<int> &natual_group, da_cfg *cfg)
{
	assert(!num_vec.empty());
	assert(npos > 0);
	assert(npos < num_vec.size());

	double evalue = -1.0;
	edge_evalue_vec[npos] = evalue;

	/**
	 * 第01部分，如果front_vec表示的指令数太少，则导致前部分粒度太小，不划分
	 */
	std::vector<int> front_vec = sub_num_vec(num_vec, 0, npos);
	int front_size = get_node_group_size(front_vec, cfg);
	if(threshold::is_small(front_size)){
		return evalue;
	}

	/**
	 * 第02部分，如果cqip_vec表示的指令数太少，则导致后部分粒度太小，不划分
	 */
	std::vector<int> cqip_vec = sub_num_vec(num_vec, npos);
	int cqip_size = get_node_group_size(cqip_vec, cfg);
	if (threshold::is_small(cqip_size)) {
		return evalue;
	}

	/**
	 * 第一部分：获取处理related_call和related_loop之后的spawn_path
	 */
	std::vector<int> spawn_path_vec = get_effect_spawn_path_vec(cqip_vec, natual_group, cfg);

	/**
	 * 第二部分：如果spawn_path长度有效，
	 *         在spawn_path上面根据评价函数选取最优的sp点
	 * 如果不能划分，返回一个指示值
	 */
	evalue = get_best_evaluation(spawn_path_vec, cqip_vec, cfg);
	edge_evalue_vec[npos] = evalue;
	return evalue;
}



/**
 * 获取cqip_vec所对应的候选线程的spawn_path
 * 如果返回NULL，表示不适合划分(激发距离太小)
 */
/*const std::vector<int> get_effect_spawn_path_vec(
		const std::vector<int> & cqip_vec, const super_block_cfg *scfg) {
	assert(!cqip_vec.empty());

	*
	 * (1).获取最大可能的激发路径范围biggest_path_vec
	 * (2).根据spawn_pos_trace *spawn_trace裁剪得到spawn_path_vec_by_trace;
	 * (3).对spawn_path_by_trace根据循环裁剪得到effective_spawn_path

	const std::vector<int> all_vec = get_num_vec_from_scfg(scfg);
	std::vector<int> biggest_path_vec = sub_num_vec_front_splited_by_elem(all_vec, cqip_vec[0]);
	//注意spawn_vec_by_trace为empty情况
	const std::vector<int> spawn_vec_by_trace = get_spawn_vec_by_spawn_trace(biggest_path_vec, cqip_vec, scfg);
	printf("spawn_vec_by_trace is :");
	test_print_num_vec(stdout, spawn_vec_by_trace, scfg);
	printf("\n");

	if(spawn_vec_by_trace.empty()){
		return spawn_vec_by_trace;
	}

	return  pruned_spawn_vec_by_related_loop(spawn_vec_by_trace, cqip_vec, scfg);
}*/
//2012修改，使得不能跨越循环
/**
 * @description - 获取有效路径
 * @param cqip_vec - 不允许为empty
 * @param natual_group,作为最大激发距离
 */
const std::vector<int> get_effect_spawn_path_vec(
		const std::vector<int> & cqip_vec, const std::vector<int> &natual_group, da_cfg *cfg) {
	assert(!cqip_vec.empty());

	/**
	 * (1).获取最大可能的激发路径范围biggest_path_vec
	 * (2).激发点不能跨越循环,根据循环来分割
	 * (3).对spawn_path_by_trace根据循环裁剪得到effective_spawn_path
	 */
	//const std::vector<int> all_vec = get_num_vec_from_scfg(scfg);
	std::vector<int> biggest_path_vec = sub_num_vec_front_splited_by_elem(natual_group, cqip_vec[0]);

	const std::vector<int> spawn_vec_by_loop = spawn_vec_back_split_by_loop(biggest_path_vec, cqip_vec, cfg);

	return get_spawn_vec_by_spawn_trace(spawn_vec_by_loop, cqip_vec, cfg);
}


/**
 * @descript - 根据spawn_pos获取cqip_vec对应线程的激发起点
 *             spawn_trace 用来分析函数调用点;
 *             我们不希望将(定值相关的函数调用指令)包含在预计算片段中。
 *             因此，希望从最后一条(定值相关的函数调用指令)之后开始
 *
 * 			   由于super_block建立时设置break_at_call = true, 所以基本块满足end_in_call，所以直接从一下块开始
 * @others  - 由于我们选择了激发路径上不能包含循环，所以biggest_path_vec没有循环
 * @ret     - 返回的const std::vector<int>可以为empty，调用方需要验证粒度
 */
const std::vector<int> get_spawn_vec_by_spawn_trace(const std::vector<int> &biggest_path_vec,
		const std::vector<int> & cqip_vec, da_cfg *cfg)
{
	assert(!cqip_vec.empty());
	if(biggest_path_vec.empty()){
		return biggest_path_vec;
	}

	//spawn path为path_vec的子集，如果path_vec太小，则不值得激发。
	if (get_node_group_size(biggest_path_vec, cfg)
			< threshold::nloop_spawning_distance_lower) {
		return biggest_path_vec;
	}

	/**
	 * 1.数据流分析对path_vec裁剪,得到精确的spawn_path
	 */
	bit_set liveins;
	super_block *subgraph = get_super_block_list_by_vec(cqip_vec, cfg);
	cfg->find_liveins(subgraph, the_bit_mgr, &liveins); //globe-var the_cfg , the_bit_mgr

	//先由spawn_pos_trace确定
	spawn_pos_trace *spawn_trace = new spawn_pos_trace();
	bit_set *spawn_related = new bit_set(0, the_reach->catalog()->num_defs());
	spawn_related->clear();

	spawn_trace->construct_spawn_pos_trace(biggest_path_vec, cfg);
	search_related_point(&liveins, spawn_trace, spawn_related);

	spawn_trace->analyze_related_cals(the_reach, spawn_related);
	std::vector<machine_instr *> *cal_instrs = spawn_trace->get_related_cals();
	if (!cal_instrs->empty()) { //确定最后一次的call指令，因为break_at_call，所以直接从下一个节点开始
		std::vector<machine_instr *>::reverse_iterator riter =
				cal_instrs->rbegin();
		machine_instr *last_cal_instr = *riter;
		tree_instr *ti = last_cal_instr->parent();
		tnle * t_e = ti->list_e();
		cfg_node *cn = get_cfg_node_by_tnle(t_e);
		int call_sblock__num = cn->number();
		return sub_num_vec_back_splited_by_elem(biggest_path_vec, call_sblock__num);
	} else {
		return biggest_path_vec;
	}
}

/**
 * @description 根据spawn_vec上loop分割，取后端
 * @others spawn_vec可以为empty
 * @ret 可以为empty
 */
const std::vector<int> spawn_vec_back_split_by_loop(
		const std::vector<int> &spawn_vec, const std::vector<int> &cqip_vec,
		da_cfg *cfg) {
	assert(!cqip_vec.empty());
	if (spawn_vec.empty()) {
		return spawn_vec;
	}

	if (get_node_group_size(spawn_vec, cfg)
			< threshold::nloop_spawning_distance_lower) {
		return spawn_vec;
	}

	int last_num = get_last_loop_num(spawn_vec, cfg);

	if (last_num > 0) {
		return sub_num_vec_back_splited_by_elem(spawn_vec, last_num);
	} else {
		return spawn_vec;
	}
}




/**
 * @description - 返回spawn_vec中的最后一个loop的num，不存在则返回-1
 * @ret num if exited, else -1
 */
int get_last_loop_num(const std::vector<int> & spawn_vec,
		da_cfg *cfg) {
	assert(!spawn_vec.empty());
	int iRet = -1;

	for (int i = 0; i < spawn_vec.size(); ++i) {
		if(cfg->is_loop_begin(spawn_vec[i])){
			iRet = spawn_vec[i]; //不要break，要取最后一个
		}
	}

	return iRet;
}



/**
 * @param spawn_vec 允许empty
 * @param cqip_vec  不允许为empty
 */
double get_best_evaluation(const std::vector<int> &spawn_vec, const std::vector<int> &cqip_vec, da_cfg *cfg){
	/**
	 * 不需要获取spawn_pos,所以传入NULL
	 * 准确的说, 是不用spawn_pos 的返回值.
	 */
	tnle *spawn_pos = NULL;
	return get_best_evaluation_and_spawn_pos(spawn_vec, cqip_vec, spawn_pos, cfg);
}

/**
 * 在spawn_vec中寻找sp点，寻找过成先要满足一定的约束条件，又要使评价函数值最大化
 * 否则返回 < 0, 并且	spawn_pos = NULL
 * @param spawn_vec - 可以为empty
 * @return 1. 返回该spawn_path 上可以取得的最优评价. 以返回值的形式进行返回.  2. 返回取得该最优评价的spawn_pos, 以引用参数spawn_pos的形式返回.
 */
double get_best_evaluation_and_spawn_pos(const std::vector<int> & spawn_vec,
		const std::vector<int> & cqip_vec, tnle *&spawn_pos,
		da_cfg *cfg)
{
	assert(!cqip_vec.empty());

	//test_print_num_vec(stdout, spawn_vec, scfg);
	double dbRet = -1.0;
	spawn_pos = NULL;
	if (spawn_vec.empty()) {
		return dbRet;
	}




	//用来寻找sp点 和 用来计算激发距离
	super_block *non_loop_spawn_path =
			super_block::get_nloop_spawn_super_block_by_vec(spawn_vec, cfg);

	//数据流分析
	bit_set liveins;
	super_block *subgraph = get_super_block_list_by_vec(cqip_vec, cfg);
	the_cfg->find_liveins(subgraph, the_bit_mgr, &liveins); //globe-var the_cfg , the_bit_mgr
	bit_set *path_related = new bit_set(0, the_reach->catalog()->num_defs());
	path_related->clear();
	search_related_point(&liveins, non_loop_spawn_path, path_related);

	//我们选择评价值最大的返回
	double iter_evalue = -1.0;  //对应spawn_pos
	tnle *iter_spawn_pos = NULL;

	int spawn_distance = 0;
	bit_set *ins_dep = new bit_set(0, the_reach->catalog()->num_defs());
	int total_count = non_loop_spawn_path->static_size();
	for (int index = 0; index < total_count; ++index) {
		/**
		 * 从第一条指令开始，看是否满足一下条件
		 * 1.dependence_threshold
		 *
		 */
		machine_instr *spawn_pos_instr = non_loop_spawn_path->instr_access(
				index);
		assert(NULL != spawn_pos_instr);

		iter_spawn_pos = spawn_pos_instr->parent()->list_e();

#define KM_DEBUG
#ifdef KM_DEBUG
		fprintf(stderr, "\nopcode is: \t%s\n", spawn_pos_instr->op_string());
		std::cerr << spawn_pos_instr->parent()->number() << std::endl;
		tree_instr * tr =  spawn_pos_instr->parent();
		std::cerr << "block number :" << non_loop_spawn_path->in_which_block(iter_spawn_pos)->number() <<  std::endl;

#endif
		*ins_dep = *path_related;
		pruning_instrs_before_spawn(ins_dep, non_loop_spawn_path, iter_spawn_pos);
		//经过上个函数, ins_dep包含了从spawn到cqip之间的定义点指令(定义下一个线程的live_in instructions).

		/*int cnt1 = path_related->count(); // for test
		 int cnt2 = ins_dep->count(); // for test*/

		spawn_distance = compute_spawning_distance_from_sp(non_loop_spawn_path, spawn_pos_instr);
		int dep_cnt = ins_dep->count();
		if (dep_cnt <= (int) threshold::nloop_dependence_threshold
				&& spawn_distance >= threshold::nloop_spawning_distance_lower
				&& spawn_distance <= threshold::nloop_spawning_distance_upper
				&& spawn_distance - dep_cnt > 0) {
			iter_evalue = spawn_distance - dep_cnt;
			//取最大值，记下来
			if(iter_evalue >= dbRet){
				dbRet = iter_evalue;
				spawn_pos = iter_spawn_pos;
			}
		}

	}

	//for_test
/*	if(dbRet > 0){
		fprintf(test_fp, "有效激发提前时间:%d.\n", (int)dbRet);
	}*/


	delete non_loop_spawn_path;
	delete subgraph;
	delete path_related;
	delete ins_dep;

	return dbRet;
}


/**
 * 为head中的所有非Loop的node_group寻找和设置sp点
 * 激发路径在natual_group中
 */
void set_sp_for_all_node_group(util::node_list *head, const std::vector<int> &natual_group, da_cfg *cfg) {
	util::node_list_iter nli(head);

	printf("\n线程划分结果如下：\n");
	while (!nli.is_empty()) {
		util::node *pNode = nli.step();
		if (pNode->is_node_group()) {
			std::vector<int> group = pNode->getNode_group();
			printf("    node group block number vector:");
			test_print_num_vec(stdout, group, cfg);
#define KM_DEBUG
#ifdef KM_DEBUG
			fprintf(stderr, "\n, natual_group :\n");
			int the_size = 0;
			the_size = natual_group.size();
			for(int i = 0; i < the_size; i++){
				fprintf(stderr, "%d \t", natual_group[i]);
			}
			fprintf(stderr, "\n");
#endif
			//set sp for group
			tnle *sp = get_spawn_pos(group, natual_group, cfg);
			pNode->setSpawn_pos(sp);

			printf("    sp信息：\n");
			if (NULL != sp) {
				//打印sp信息
				int num = get_cfg_node_by_tnle(sp)->number();
				printf("sp at block_%d, \n", num);
				fflush(stdout);
				sp->contents->print(stdout);
				fflush(stdout);
			} else {
				printf("NULL\n");
			}
			fflush(stdout);
		}
	}
}

/**
 *
 * @param cqip_vec    cqip可也插入在这些节点上.
 * @param natual_group
 * @param cfg
 * @return
 */
tnle *get_spawn_pos(const std::vector<int> & cqip_vec, const std::vector<int> &natual_group,
		da_cfg *cfg)
{
	/*其中处理过程类似evalue_at_given_pos
	 * 1.获取spawn_path   --
	 * 2.获取使评估函数最大的sp点
	 */
	std::vector<int> spawn_vec = get_effect_spawn_path_vec(cqip_vec, natual_group, cfg);

#define KM_DEBUG
#ifdef KM_DEBUG
int spawn_vec_size = spawn_vec.size();
for(int i = 0; i < spawn_vec_size; i++){
	fprintf(stderr, "%d \t", spawn_vec[i]);
}
fprintf(stderr, "\n");
#endif

	tnle *spawn_pos = NULL;
	if (!spawn_vec.empty()) {
		//get_best_evaluation_and_spawn_pos(spawn_vec, cqip_vec, spawn_pos, scfg);

		get_best_evaluation_and_spawn_pos(spawn_vec, cqip_vec, spawn_pos, cfg);
	}



	return spawn_pos;
}


void construct_nloop_thread(util::node_list *head,
		const super_block_cfg *scfg) {
	util::node_list_iter nli(head);

	while (!nli.is_empty()) {
		util::node *pNode = nli.step();
		if (pNode->is_node_group()) {
#define KM_DEBUG
#ifdef KM_DEBUG
	pNode->print(stderr);
	fprintf(stderr, "\n******************\n");
#endif
			thread *t = thread::construct_thread_by_node_group(pNode, scfg);
			t->set_thread_type(thread::NONSPECULATIVE);
			finish_construction(t);
		}
	}
}


void destruct_node_list(util::node_list * &head) {
	if (NULL != head) {
		util::node_list_iter nli(head);

		while (!nli.is_empty()) {
			util::node *pNode = nli.step();
			if (NULL != pNode) {
				delete pNode;
				pNode = NULL;
			}
		}

		delete head;
		head = NULL;
	}
}




//---------------------------------discard------------------------------------


/**
 * @description 返回最后一个related_loop的num, 如果不存在，则返回-1
 *              related_loop表示，有包含定值点的loop
 * @others    使用全局变量  reaching_def_problem *the_reach, the_scfg
 */
/*int get_last_related_loop_num(const std::vector<int> & spawn_vec,
		bit_set *related, da_cfg *cfg) {
	assert(!spawn_vec.empty());
	assert(NULL != related);

	int iRet = -1;

	for (int i = 0; i < spawn_vec.size(); ++i) {
		super_block *sb = scfg->get_super_block_by_num(spawn_vec[i]);
		if (sb->is_loop()) {
			loop_block *loop = (loop_block *) sb;

			bit_set_iter iter(related);
			while (!iter.is_empty()) {
				int n = iter.step();
				machine_instr *mi =
						(machine_instr *) the_reach->catalog()->lookup(n);

				if (loop->contains(mi)) {
					iRet = loop->block_num();
					printf("related instr in loop is:\n");
					mi->print(stdout);
					printf("\n");
					continue;
				}
			}
		}
	}

	return iRet;
}*/



/**
 * @description - 根据relted_loop裁剪spawn_vec
 *                检测所有（定值点和相关点ralated_）,如果存在loop中，则选取loop之后的子路径返回
 *                因为在loop中的定值点我们不能预测
 * @ret           返回裁剪后的spawn_vec - 可能为empty
 */
/*const std::vector<int> pruned_spawn_vec_by_related_loop(
		const std::vector<int> &spawn_vec, const std::vector<int> &cqip_vec,
		da_cfg *cfg) {
	assert(!cqip_vec.empty());
	if(spawn_vec.empty()){
		return spawn_vec;
	}

	if (get_node_group_size(spawn_vec, cfg)
			< threshold::nloop_spawning_distance_lower) {
		return spawn_vec;
	}

	//构造spawn_path
	super_block *spawn_path = super_block::get_nloop_spawn_super_block_by_vec(spawn_vec, cfg);

	//数据流分析
	bit_set liveins;
	super_block *subgraph = get_super_block_list_by_vec(cqip_vec, cfg);
	the_cfg->find_liveins(subgraph, the_bit_mgr, &liveins); //globe-var the_cfg , the_bit_mgr
	bit_set *path_related = new bit_set(0, the_reach->catalog()->num_defs());
	path_related->clear();
	search_related_point(&liveins, spawn_path, path_related);

	printf("path_related is:\n");
	path_related->print(stdout);
	printf("\n");

	int last_num = get_last_related_loop_num(spawn_vec, path_related, cfg);

	delete path_related;
	if (last_num > 0) {
		return sub_num_vec_back_splited_by_elem(spawn_vec, last_num);
	} else {
		return spawn_vec;
	}
}*/
