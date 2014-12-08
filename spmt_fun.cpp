/*
 * spmt_fun.cpp
 *
 *  Created on: 2012-3-12
 *      Author: harry
 */

#include "spmt_fun.h"
#include "common.h"
#include "misc.h"
#include "spmt_instr.h"
#include "loop_block.h"
#include "da_cfg.h"
#include "threshold.h"

#include <cassert>
#include <cstddef>
#include <list>
#include <algorithm>

extern std::vector<thread *>*threads;

static int begin_end_path_flag = 0;			//addbyekeyming 20140623
static std::vector<super_block_path*> *path_list = NULL;		//addbyekeyming 20140623
class multi_path;
multi_path *mp = NULL;


//addbykeyming 20140624  node_num_with_flag
struct node_num_with_flag{
	super_block *sblock;
	int node_num;
	bool the_all_flag;
	bool taken_flag;
	bool fall_flag;
	node_num_with_flag():sblock(NULL), node_num(0), the_all_flag(0), taken_flag(0), fall_flag(0){}
	node_num_with_flag(super_block *sb, int num):sblock(sb), node_num(num), the_all_flag(0), taken_flag(0), fall_flag(0){}
	void clearAll(){
		the_all_flag = 0;
		taken_flag = 0;
		fall_flag = 0;
	}
	void set_taken_flag(){
		taken_flag = 1;
	}
	void set_all_flag(){
		the_all_flag = 1;
	}
	void set_fall_flag(){
		fall_flag = 1;
	}
	bool all_flag(){
		if(taken_flag == 1 && fall_flag == 1){
			return true;
		}
		return false;
	}
	void print_with_num()const{
		fprintf(stderr, "\t%d\t%d\t%d\t%lf\n", node_num, taken_flag, fall_flag, sblock->taken_prob());
	}
};

//addbykeyming 20140626
void print_node_num_stack(const std::vector<node_num_with_flag> &path_num){
	int stack_size = path_num.size();
	fprintf(stderr, "Begin to print Stack.\nStack size is %d\n", stack_size);
	for(int i = 0; i < stack_size; i++){
		path_num[i].print_with_num();
	}
	fprintf(stderr, "\nEnd print Stack.\n");
}

//addbykeyming 20140625 class multi_path
class multi_path{
private:
	super_block * begin;
	super_block * end;
	int pos;

public:
	multi_path(super_block *b, super_block *e);
	super_block_path* get_one_path();
	super_block* get_begin(){return begin;}
	super_block* get_end(){return end;}
	void print_multi_path();
};

multi_path::multi_path(super_block *b, super_block *e):begin(b), end(e), pos(0){
	assert(b);
	assert(e);
	find_most_likely_multi_path(begin, end);
	begin_end_path_flag = 0;
	print_multi_path();
}

void multi_path::print_multi_path(){
	int size = path_list->size();
	fprintf(stderr, "Begin print multi path\n From , to: %d\t %d\n Has path %d\n", begin->block_num(), end->block_num(), path_list->size());
	for(int i = 0; i < size; i++){
		(*path_list)[i]->print_block_list_num(stderr);
	}
	fprintf(stderr, "End print multi path\n");
}
inline super_block_path* multi_path::get_one_path() {
	assert(pos >= 0);
	fprintf(stderr, "path number is %d\n", path_list->size());
	if(pos < path_list->size()){
		fprintf(stderr, "The path is as below:\n");
		(*path_list)[pos]->print_block_list_num(stderr);
		return (*path_list)[pos++];
	}
	else{
		return NULL;
	}
}



//addbykeyming 20140624 copy_to_path_list.
static void copy_to_path_list(std::vector<node_num_with_flag> &path_num, super_block_path* path);



static super_block_path *find_taken_likely_path(super_block * begin,
		super_block * end);

static super_block_path *find_fall_likely_path(super_block * begin,
		super_block * end);

static super_block_path *find_most_likely_path_by_coverage(super_block * begin,
		super_block * end);

void copy_to_path_list(std::vector<node_num_with_flag> &path_num, super_block_path* path)
{
	int size = path_num.size();
	for(int i = 1; i < size; i++){
		path->add_super_block(path_num[i].sblock);
	}
}



/** for min_cut.h and main.cpp
 * @param begin - super block*
 * @param end - super block*
 * @return super_block_path -  most likely path in regin (begin, end)
 */
super_block_path *find_most_likely_path(super_block * begin, super_block * end){

	return find_most_likely_path_by_coverage(begin, end);
}


/** addbykeyming 20140623
 * 对于从begin到end(两个相邻的函数支配节点)的路径 包含sp_block的情况, 找其路径时, 进行特殊处理.
 * 前置断言, begin 不是sp_block
 * @param begin
 * @param end
 * @return
 */
super_block_path *find_most_likely_path_with_sp_block(super_block * begin, super_block * end){

	//return find_most_likely_path_by_coverage(begin, end);
	assert(NULL != begin);
	assert(NULL != end);

	super_block_path *path = NULL;

	if(mp == NULL){
		mp = new multi_path(begin, end);
	}
	if(mp->get_begin() != begin || mp->get_end() != end){
		delete mp;
		mp = new multi_path(begin, end);
	}
	path = mp->get_one_path();
	return path;
}



std::vector<super_block_path*>* find_most_likely_multi_path(super_block * begin, super_block * end){
	assert(NULL != begin);
	assert(NULL != end);


	if(begin_end_path_flag == 0){
		if(path_list != NULL){
			delete path_list;

		}
		path_list = new std::vector<super_block_path*>;

		std::vector<node_num_with_flag> path_num;

		super_block *sb = begin;
		while(sb != end){
			node_num_with_flag the_node(sb, sb->block_num());
			if(sb->taken_multi_suit()){
				the_node.set_taken_flag();
				sb = sb->take_succ();
			}
			else if(sb->fall_multi_suit()){
				the_node.set_fall_flag();
				sb = sb->fall_succ();
			}

			path_num.push_back(the_node);		//path_num里不包含end 节点
		}
		super_block_path *the_path = new super_block_path();
		copy_to_path_list(path_num, the_path);
		path_list->push_back(the_path);

		print_node_num_stack(path_num);

		while(!path_num.empty()){
			super_block_path *path = new super_block_path();

			node_num_with_flag &e_node = path_num.back();
			if(e_node.all_flag()){
				path_num.pop_back();
			}
			else{
				super_block *the_block = NULL;
				node_num_with_flag bra_node;
				do{
					node_num_with_flag &e_node = path_num.back();
					if(!e_node.taken_flag){						//需要设置概率不够时的  flag值.?????
						if(!e_node.sblock->taken_multi_suit()){
							e_node.set_taken_flag();
							//path_num.pop_back();
							break;
						}
						e_node.set_taken_flag();
						the_block= e_node.sblock->take_succ();
						node_num_with_flag next_node(the_block, the_block->block_num());
						//next_node.set_taken_flag();
						bra_node = next_node;
					}
					else if(!e_node.fall_flag){
						if(!e_node.sblock->fall_multi_suit()){
							e_node.set_fall_flag();
							//path_num.pop_back();
							break;
						}
						e_node.set_fall_flag();
						the_block = e_node.sblock->fall_succ();
						node_num_with_flag next_node(the_block, the_block->block_num());
						//next_node.set_fall_flag();
						bra_node = next_node;
					}
					else{
						assert(!"forget to set the all flag when have set taken_flag and fall_flag");
					}

					if(the_scfg->is_in_sp_block_map(bra_node.node_num)){
						break;
					}

					if(bra_node.sblock != end){
						path_num.push_back(bra_node);
					}
					else{
						break;
					}
				}while(the_block != end);

				if(the_block == end){
					print_node_num_stack(path_num);
					copy_to_path_list(path_num, path);
					path_list->push_back(path);
				}

			}

		}

		begin_end_path_flag = 1;
	}

	return path_list;

}


super_block_path *find_most_likely_path_by_coverage(super_block * begin,
		super_block * end) {
	assert(NULL != begin);
	assert(NULL != end);

	super_block_path *path = new super_block_path();
	super_block *sb = begin;

	while (sb != end) {
		double taken_prob = sb->taken_prob();
		fprintf(stderr, "The taken_prob is %lf\n", taken_prob);			//addbykeyming 20140626
		if (threshold::is_path_definite(taken_prob)) {
			//普通确定路径划分
			if (threshold::is_taken_definite(taken_prob)) {
				sb = sb->take_succ();
			} else {
				sb = sb->fall_succ();
			}

		} else {
			//计算期望指令数计算
			super_block *pdom = sb->immed_pdom();
			super_block_path *taken_likely_path = find_taken_likely_path(sb, pdom);
			super_block_path *fall_likely_path = find_fall_likely_path(sb, pdom);

			int weighted_taken_size = sb->taken_prob()* taken_likely_path->static_size();
			int weighted_fall_size = (1.0 - sb->taken_prob()) * fall_likely_path->static_size();
			if(0 == weighted_taken_size || 0 == weighted_fall_size){

				if(fall_likely_path->static_size() >= threshold::nloop_thread_size_lower){
					sb = sb->fall_succ();
				}else if(taken_likely_path->static_size() >= threshold::nloop_thread_size_lower){
					sb = sb->take_succ();
				}else{
					if(taken_prob > 0.5){
						sb = sb->take_succ();
					}else{
						sb = sb->fall_succ();
					}
				}
			}else if (weighted_taken_size > weighted_fall_size) {
				sb = sb->take_succ();
			} else {
				sb = sb->fall_succ();
			}

			delete taken_likely_path;
			delete fall_likely_path;
		}

		if (sb != end) {
			path->add_super_block(sb);
		}
	}

	return path;
}


super_block_path *find_taken_likely_path(super_block * begin,
		super_block * end) {
	assert(NULL != begin);
	assert(NULL != end);
	assert(begin != end);

	super_block_path *path = new super_block_path();
	super_block *sb = begin->take_succ();

	if (sb != end) {
		path->add_super_block(sb);
		super_block_path *sbp = find_most_likely_path_by_coverage(sb, end);
		path->add_super_blocks(sbp->super_blocks());
		delete sbp;
	}

	return path;
}


super_block_path *find_fall_likely_path(super_block * begin,
		super_block * end) {
	assert(NULL != begin);
	assert(NULL != end);
	assert(begin != end);

	super_block_path *path = new super_block_path();
	super_block *sb = begin->fall_succ();

	if (sb != end) {
		path->add_super_block(sb);
		super_block_path *sbp = find_most_likely_path_by_coverage(sb, end);
		path->add_super_blocks(sbp->super_blocks());
		delete sbp;
	}

	return path;
}


/**
 * [begin, end]
 */
super_block_path *find_most_likely_path_include_two_ends(super_block * begin,
		super_block * end) {
	assert(NULL != begin);
	assert(NULL != end);

	super_block_path *path = new super_block_path();
	if(begin != end){
		path->add_super_block(begin);

		super_block_path *sbp = find_most_likely_path_by_coverage(begin, end);
		path->add_super_blocks(sbp->super_blocks());
		delete sbp;
	}

	path->add_super_block(end);

	return path;
}


unsigned int compute_spawning_distance_from_sp(super_block * spawn_path,
		machine_instr * spawn_instr){
	unsigned int size = 0;
	cfg_node_list_iter iter(spawn_path->nodes());

	while (!iter.is_empty()) {
		cfg_node *curr_node = iter.step();
		if (curr_node->is_block()) {
			cfg_block *block = (cfg_block *) curr_node;
			if (block->contains(
					((instruction *) spawn_instr)->parent()->list_e())) {
				size += block_dynamic_size_from_instr(block, spawn_instr);
			} else if (size == 0) {
				continue;
			} else {
				size += block_dynamic_size(block);
			}
		}
	}
	return size;
}


unsigned int compute_spawning_distance_between_sp_cqip(super_block *spawn_path,
		machine_instr *spawn_instr, machine_instr *cqip_instr) {
	assert(NULL != spawn_instr);
	assert(NULL != cqip_instr);
	assert(spawn_path->contains(spawn_instr));
	assert(spawn_path->contains(cqip_instr));

	unsigned int size = 0;
	bool is_begin = false;
	cfg_node_list_iter iter(spawn_path->nodes());

	while (!iter.is_empty()) {
		cfg_node *curr_node = iter.step();

		cfg_node_instr_iter cb_instr_iter(curr_node);
		while (!cb_instr_iter.is_empty()) {
			tree_instr *ti = cb_instr_iter.step();
			machine_instr *mi = (machine_instr *) ti->instr();
			//从mi_instr指令算起，它是块中的第一条指令
			if (mi == spawn_instr) {
				is_begin = true;
			} else if (mi == cqip_instr) {
				is_begin = false;
			}

			if (is_begin) {
				if ((int) mi->opcode() == mo_null
						|| (int) mi->opcode() == mo_lab
						|| (int) mi->opcode() == mo_loc) {
					continue;
				} else if ((int) mi->opcode() == mo_jalr) {
					immed_list *iml = (immed_list *) (mi->peek_annote(
							k_call_overhead));
					if (iml) {
						int overhead = (*iml)[0].integer(); //指的是被调用的指令条数
						if (overhead == 0) {
							size++;
						} else {
							size += overhead;
						}
					} else {
						size++;
					}
				} else {
					size++;
				}
			}
		}
	}

	assert(size != 0);
	return size;
}



/**
 *  *pruning_instrs_before_spawn() --- prune the latest define point of the instructions before spawn position in the previous thread.
 裁剪spawn点之前的指令的最近定义点，这个函数的输入是定义点的指令，不是所有的指令
 输入发起线程的两个cqip点之间的所有的定义点指令，然后将spawn点之前的定义点指令去除
 * @param defs[in & out] 输入时, 是上一个线程的cqip点 和本线程的cqip点之间的所有定义点指令.    本函数的输出也是这个函数, 输出时, defs包含spawn点到cqip点之间的定义点.
 * 					     也就是, 本函数的作用是去掉了spawn点之前的定义指令.
 * @param path
 * @param spawn_pos
 */
void pruning_instrs_before_spawn(bit_set * defs, super_block * path, tnle * spawn_pos)
{
	cfg_node_list_iter cnli(path->nodes());
	while (!cnli.is_empty()) {
		cfg_node *cn = cnli.step();
		if (cn->is_block()) {
			cfg_block *block = (cfg_block *) cn;
			cfg_node_instr_iter cnii(block);
			while (!cnii.is_empty()) {
				tree_instr *test = cnii.step();
				if (test->list_e() == spawn_pos)
					return;
				else {
					//返回test->instr()这条指令中的第一个定义点
					int first_id = the_reach->catalog()->first_id(test->instr());
					//返回test->instr()这条指令中定义点的数量
					int num = the_reach->catalog()->num_ids(test->instr());
					for (int i = 0; i < num; i++){
						//设置first_id + i位置为0
						defs->remove(first_id + i);
					}

				}
			}
		}
	}
}






/*
 *finish_construction() --- insert cqip instruction and construct pre-compute sclie
 插入cqip指令并且构建预计算片段
 */
void finish_construction(thread * curr_thread) {

	tnle *spawn_pos = curr_thread->get_spawned_pos();
	if (spawn_pos == 0){
		return;
	}

	if (is_existed_curr_thread(threads, curr_thread))
	{
		return;
	}
	//选取当前线程的第一个超级块赋值给cqip
	super_block *cqip_sblock = curr_thread->first_super_block();
	cfg_block *cqip_block = 0;
	tnle *cqip_instr = 0;
	if (cqip_sblock->knd() == super_block::BLOCK) //如果
	{
		cqip_block = (cfg_block *) cqip_sblock->first_block();
		cqip_instr = insert_cqip_instr(cqip_block, false);
	} else if (cqip_sblock->knd() == super_block::LOOP) {
		/*loop_block *loop = (loop_block *) cqip_sblock;
		cqip_block = (cfg_block *) loop->entry_node();
		cqip_instr = insert_cqip_instr_for_loop(cqip_block, true);*/
		assert(false);
	}

	label_sym *cqip_pos_num = peek_cqip_pos(cqip_instr);
	printf("\ncqip_pos_num label is:\n");
	cqip_pos_num->print(stdout);
	printf("\n");

	super_block *spawn_sblock = the_scfg->in_which_super_block(spawn_pos);
	cfg_block *spawn_block = (cfg_block *) spawn_sblock->first_block();

	super_block *pslice_path = find_pslice_path(spawn_sblock, cqip_sblock);

	//妈的，又重新计算了一次和 find_optimal_dependence中一样
	bit_set liveins;
	the_cfg->find_liveins(curr_thread->convert_to_super_block(), the_bit_mgr,
			&liveins);

	bit_set *ins_dep = new bit_set(0, the_reach->catalog()->num_defs());
	//search_pslice_instrs(&liveins, pslice_path, spawn_pos, ins_dep);
	search_related_point(&liveins, pslice_path, ins_dep);     // replaced by SYJ 2012-03-12
	//裁剪sp之前的指令
	pruning_instrs_before_spawn(ins_dep, pslice_path, spawn_pos);

	tree_node_list *pslice = new tree_node_list;
	construct_pslice(ins_dep, cqip_pos_num, pslice);
	printf("pslice is:\n");
	pslice->print(stdout);
	printf("\n");

	//
	curr_thread->set_cqip_pos(cqip_instr);
	curr_thread->set_cqip_block(cqip_block);

	//3month 8days
	//cqip_block->insert_after(pslice, cqip_instr->next());
	//insert_spawn_instr(spawn_block, spawn_pos, cqip_pos_num);
	//
	curr_thread->set_spawn_info(spawn_block, pslice, cqip_pos_num);

	add_thread(threads, curr_thread);
	delete pslice_path;
}


/*
 *add_thread() --- If current thread isn't contained in vector of thread, add current thread to vector of thread
 如果当前线程不在线程容器中，则将他添加进入线程容器*/
void add_thread(std::vector<thread *>*threads, thread * curr_thread) {
	if (std::find(threads->begin(), threads->end(), curr_thread)
			!= threads->end())
		return;
	else
		threads->push_back(curr_thread);
}



/*
 *在线程容器中找当前线程是否存在
 */
bool is_existed_curr_thread(std::vector<thread *>*threads, thread * curr_thread) {
	if (std::find(threads->begin(), threads->end(), curr_thread)
			!= threads->end())
		return true;
	else
		return false;
}


/*
 *construct_pslice() --- construct pre-compute slice.
 */
tree_node_list *construct_pslice(bit_set * ins_dep, label_sym * cqip_pos,
		tree_node_list * pslice) {
	//generate the entry of the pre-compute slice
	immed_list *entry_iml = new immed_list; //immed为立即数，这里构造一个立即数链
	entry_iml->append((immed) cqip_pos); //将cqip_pos追加到立即数链
	machine_instr *pslice_entry_instr = new mi_bj(mo_pslice_entry, cqip_pos); //构造入口的机器指令
	pslice_entry_instr->set_annote(k_pslice_entry_pos, entry_iml); //为立即数链添加注解
	/*printf("cqip_pos:");
	cqip_pos->print(stdout);
	printf("\n");
	printf("ins_dep:");
	ins_dep->print(stdout);
	printf("\n");*/

	bit_set_iter iter1(ins_dep);
	while (!iter1.is_empty()) {
		int n = iter1.step();
	/*	printf("liveins");
		printf("%d", n);
		printf("exist in instruction：");
		((machine_instr *) the_reach->catalog()->lookup(n))->print(stdout);
		printf("+++");*/
	}
/*	printf("\n");
	printf("pslice_entry_instr:");
	pslice_entry_instr->print(stdout);
	printf("\n");*/
	tree_instr *entry_instr = new tree_instr(pslice_entry_instr); //构造suif指令（机器无关指令）

	/*printf("pslice_entry_instr_tree_instr:");
	 entry_instr->print(stdout);
	 printf("\n");
	 printf("pslice_entry_instr_tree_instr_instr:");
	 entry_instr->instr()->print(stdout);
	 printf("\n");
	 */
	pslice->append(entry_instr); //将entry_instr追加到pslice


	//according to the bit_set of ins_dep, construct the pre-compute slice
	bit_set_iter iter(ins_dep); //构造ins_dep的迭代器

	while (!iter.is_empty()) {
		int n = iter.step();
		machine_instr *mi = (machine_instr *) the_reach->catalog()->lookup(n); //the_reach
		if (mi != (machine_instr *) the_reach->catalog()->lookup(n - 1)) {

			//指令是mo_lab指令什么都不做
			if ((int) mi->opcode() == mo_lab) {
				//do noting
			}
			//mif_xx类型的指令伪操作指令什么都不做
			else if (((int) mi->opcode() >= mo_null
					&& (int) mi->opcode() <= mo_half)
					|| (int) mi->opcode() == mo_file
					|| (int) mi->opcode() == mo_endr) {
				//do noting
			}
			//modified in 3.20  = mo_la
			//mif_rr类型指令
			else if (((int) mi->opcode() >= mo_rol
					&& (int) mi->opcode() <= mo_sne)
					|| ((int) mi->opcode() >= mo_la
							&& (int) mi->opcode() <= mo_movt)
					|| ((int) mi->opcode() >= mo_nop
							&& (int) mi->opcode() <= mo_li_s)
					|| (int) mi->opcode() == mo_fst
					|| (int) mi->opcode() == mo_ust) {
				if ((int) mi->opcode() == mo_la) //call指令
				{
					if (mi->src_op(0).is_reg() && (int) mi->src_op(0).reg() == 29) //如果指令的第一个操作数是寄存器且所用寄存器为29即栈指针
						continue;
				}

				/**
				 * 如果是乘法指令，
				 * mult x, y
				 * mflo z
				 * 我们需要将其前一条指令也加入pslice中
				 */
				if ((int) mi->opcode() == mo_mflo || (int) mi->opcode() == mo_mfhi) {
					machine_instr *pre_mi = get_immediate_pre_instr_in_block(mi);
					if ((int)pre_mi->opcode() == mo_mult || (int)pre_mi->opcode() == mo_multu) {
						machine_instr *mi_if_rr = new mi_rr();
						base_symtab *dscope = pre_mi->parent()->scope();
						replacements r;
						pre_mi->find_exposed_refs(dscope, &r);
						r.resolve_exposed_refs(dscope);
						mi_if_rr = (machine_instr *) pre_mi->clone_helper(&r);
						tree_instr *ti = new tree_instr(mi_if_rr);
						pslice->append(ti);
					}

				}

				machine_instr *mi_if_rr = new mi_rr();
				base_symtab *dscope = mi->parent()->scope();
				replacements r;
				mi->find_exposed_refs(dscope, &r);
				r.resolve_exposed_refs(dscope);
				mi_if_rr = (machine_instr *) mi->clone_helper(&r);
				tree_instr *ti = new tree_instr(mi_if_rr);

				pslice->append(ti);

			}
			/*mif_bj类型指令 跳转指令，分两种跳转，*
			 * 一种是分支跳转
			 * 另一种是函数调用跳转：jalr, jal , jr； 这三种指令不能包括在预计算片段中
			 */
			else if (((int) mi->opcode() >= mo_b
					&& (int) mi->opcode() <= mo_bnez)
					|| ((int) mi->opcode() >= mo_beq
							&& (int) mi->opcode() <= mo_jr)
					|| (int) mi->opcode() == mo_bc1f
					|| (int) mi->opcode() == mo_bc1t) {
				//函数调用跳转指令不能包括在P-slice中
				if((int) mi->opcode() == mo_jalr ||
						(int) mi->opcode() == mo_jal ||(int) mi->opcode() == mo_jal){
					continue;
				}

				machine_instr *mi_if_bj = new mi_bj();
				base_symtab *dscope = ((mi_bj *) mi)->parent()->scope();
				replacements r;
				mi->find_exposed_refs(dscope, &r);
				r.resolve_exposed_refs(dscope);
				mi_if_bj = (machine_instr *) mi->clone_helper(&r);
				tree_instr *ti = new tree_instr(mi_if_bj);

				/*printf("跳转指令_machine_instruction:");
				 mi_if_bj->print();
				 printf("\n");*/
				//printf("跳转指令(instr)：");
				//ti->instr()->print(stdout);
				//printf("\n");
				//printf("跳转指令（tree_instr）:");
				//ti->print(stdout);
				//printf("\n");
				pslice->append(ti);

			}

		}

	}

	//generate the exit of the pre-compute slice
	immed_list *exit_iml = new immed_list;
	exit_iml->append(immed(cqip_pos));
	machine_instr *pslice_exit_instr = new mi_bj(mo_pslice_exit, cqip_pos);
	pslice_exit_instr->set_annote(k_pslice_exit_pos, entry_iml);
	tree_instr *exit_instr = new tree_instr(pslice_exit_instr);

	/*printf("（machine_instr）pslice退出：");
	 pslice_exit_instr->print();
	 printf("\n");*/
	/*printf("（instr）pslice退出：");
	 exit_instr->instr()->print(stdout);
	 printf("\n");
	 printf("（tree_instr）pslice退出：");
	 exit_instr->print(stdout);
	 printf("\n");
	 */
	pslice->append(exit_instr);

	//printf("in construct pslice\n");
	return pslice;

}



/*
 *find_pslice_path() --- find pre-compute path between spawn super block and cqip super block
 */
//spawn_sblock cqip_sblock
/*super_block *find_pslice_path(super_block * spawn_sblock, super_block * cqip_sblock) {
	super_block *pslice_path = new super_block(super_block::BLOCK_LIST);
	super_block *sblock = spawn_sblock;
	while (sblock != cqip_sblock) { //如果超级块的类型为block，则将这个超级块中的所有节点都加入到pslice路径中
		if (sblock->knd() == super_block::BLOCK)
			pslice_path->add_blocks(sblock->nodes());
		//如果超级块的类型为循环，什么都不做
		else if (sblock->knd() == super_block::LOOP) {

		} else
			assert(0);
		//根据跳转的概率选取最可能的路径作为下一个要进行的超级块
		if (sblock->taken_prob() < 0.5)
			sblock = sblock->fall_succ();
		else
			sblock = sblock->take_succ();
	}

	return pslice_path;
}*/

super_block *find_pslice_path(super_block * spawn_sblock,
		super_block * cqip_sblock) {
	super_block *pslice_path = new super_block(super_block::BLOCK_LIST);
	super_block *sblock = spawn_sblock;
	while (sblock != cqip_sblock) { //如果超级块的类型为block，则将这个超级块中的所有节点都加入到pslice路径中
		if (sblock->knd() == super_block::BLOCK)
			pslice_path->add_blocks(sblock->nodes());
		//如果超级块的类型为循环，什么都不做
		else if (sblock->knd() == super_block::LOOP) {

		} else {
			assert(0);
		}

		double taken_prob = sblock->taken_prob();
		if (threshold::is_path_definite(taken_prob)) {
			//普通确定路径划分
			if (threshold::is_taken_definite(taken_prob)) {
				sblock = sblock->take_succ();
			} else {
				sblock = sblock->fall_succ();
			}
		} else {
			//计算期望指令数计算
			super_block *pdom = sblock->immed_pdom();
			super_block_path *taken_likely_path = find_taken_likely_path(sblock,
					pdom);
			//printf("taken_likely_path:\n");
			//taken_likely_path->print(stdout);
			//printf("\n");
			//printf("taken_likely_path size:%d.\n", taken_likely_path->size());
			//printf("sb taken_prob:%f.\n", sb->taken_prob());

			super_block_path *fall_likely_path = find_fall_likely_path(sblock,
					pdom);
			//printf("fall_likely_path:\n");
			//fall_likely_path->print(stdout);
			//printf("\n");

			int weighted_taken_size = sblock->taken_prob()
					* taken_likely_path->static_size();
			int weighted_fall_size = (1.0 - sblock->taken_prob())
					* fall_likely_path->static_size();

			if (0 == weighted_taken_size || 0 == weighted_fall_size) {
				if (fall_likely_path->static_size()
						>= threshold::nloop_thread_size_lower) {
					sblock = sblock->fall_succ();
				} else if (taken_likely_path->static_size()
						>= threshold::nloop_thread_size_lower) {
					sblock = sblock->take_succ();
				} else {
					if (taken_prob > 0.5) {
						sblock = sblock->take_succ();
					} else {
						sblock = sblock->fall_succ();
					}
				}
			} else if (weighted_taken_size > weighted_fall_size) {
				sblock = sblock->take_succ();
			} else {
				sblock = sblock->fall_succ();
			}

			delete taken_likely_path;
			delete fall_likely_path;
		}
	}

	return pslice_path;
}



/*
 *search_pslice_instrs() --- find the pre-compute slice instruction, contains from spawn point to control quasi-independent point,
 *                          prune the instruction before spawn point and the call instruction contained in defs
 找pslice指令，包括从sp点到cqip点之间的指令，裁剪sp点之前的指令并且包含在定义集合中的call指令

 */
bit_set *search_pslice_instrs(bit_set * liveins, super_block * spawn_path,
		tnle * spawn_pos, bit_set * defs) {
	bit_set *res = new bit_set(0, the_reach->catalog()->num_defs());
	bit_set *prev_res = new bit_set(0, the_reach->catalog()->num_defs());

	search_latest_define_point(liveins, spawn_path, res); //查找最近定义点
	/*printf("liveins最近定义点：");
	res->print(stdout);
	printf("\n");*/

	while (!res->is_empty()) {
		*defs += *res;
		*prev_res = *res;
		res->clear();
		bit_set_iter iter(prev_res); //构造prev_res的迭代器
		while (!iter.is_empty()) {
			int n = iter.step();
			machine_instr *mi = (machine_instr *) the_reach->catalog()->lookup(n); //查找对应机器指令
			//指令为mo_la
			if ((int) mi->opcode() == mo_la) {
				if (mi->src_op(0).is_instr()) {
					instruction *ins = mi->src_op(0).instr();
					if (ins->opcode() == io_ldc) {
						immed val = ((in_ldc *) ins)->value();
						if (val.is_symbol()) {
							sym_node *sn = val.symbol();
							if (sn->is_proc())
								continue;
						}
					}
				}
			}
			cfg_block *cb = find_block_contains_instr(spawn_path, mi); //查找包含mi指令的控制块

			for (int i = 0; i < mi->num_srcs(); i++) {
				operand opd = mi->src_op(i); //mi指令第i个源操作数
				/*printf("\nmachineinstruciton:");
				mi->print(stdout);*/
				//操作数为符号或寄存器
				if (opd.is_symbol() || opd.is_reg() /* || opd.is_virtual_reg() || opd.is_hard_reg() || opd.is_expr() || opd.is_instr() */) ///
				{

					/*printf("符号或寄存器操作数");
					opd.print();
					printf("\n");*/
					//int temp;
					//scanf("%d\n",&temp);
					tree_instr *t_ins = search_latest_def_point_before_instr(
							opd, mi->parent(), cb, spawn_path); //查找指定超级块之前最近的定义
					//定义存在
					if (t_ins != 0) {
						int pos = the_reach->catalog()-> first_id(
								(machine_instr *) t_ins->instr());
						res->add(pos);
					}
				}

				if (Is_ea_operand(opd)) {
					//printf("此操作数为有效地址计算！");
					instruction *in = opd.instr();
					//in->print(stdout);
					if (in->opcode() == io_add) {
						for (unsigned k = 0; k < in->num_srcs(); k++) {
							operand temp12 = in->src_op(k);

							if (temp12.is_hard_reg()) {
								//temp12.print(stdout);
								// printf("hard register\n");
								// printf("%s\n",target_regs->name(temp12.reg()));
								tree_instr *t_ins1 =
										search_latest_def_point_before_instr(
												temp12, mi->parent(), cb,
												spawn_path);
								if (t_ins1 != 0) {
									int pos = the_reach->catalog()-> first_id(
											(machine_instr *) t_ins1-> instr());
									res->add(pos);
								}

							}

						}
					}
				}
			}
		}
	}
	/*printf("\n");
	printf("裁剪之前依赖变量：");
	defs->print(stdout);
	printf("\n");
	print_bitset_with_instr(the_reach, defs);*/
	pruning_instrs_before_spawn(defs, spawn_path, spawn_pos); //截取线程激发点之前的指令
	/*printf("裁剪之后依赖变量：");
	defs->print(stdout);
	printf("\n");
	print_bitset_with_instr(the_reach, defs);*/
	pruning_cal_instrs(defs, spawn_path); //
	/*printf("裁剪call指令之后依赖变量：");
	defs->print(stdout);
	printf("\n");
	print_bitset_with_instr(the_reach, defs);*/
	return defs;
}






/*
 *pruning_cal_instrs() --- prune call instruction from defs, and delete the related instruction of the call instruction from
 *                        the defs.裁剪在定义集中的call指令，删除和call指令有关的指令
 */
void pruning_cal_instrs(bit_set * defs, super_block * spawn_path) {
	std::list<machine_instr *> instr_list;
	bit_set_iter iter(defs);
	//delete call instruction from defs
	while (!iter.is_empty()) {
		int n = iter.step();
		machine_instr *mi = (machine_instr *) the_reach->catalog()->lookup(n);

		if ((int) mi->opcode() == mo_la) //If the machine instruction is call包含这个定义点的是一条call指令
		{
			if (mi->src_op(0).is_instr()) //如果原操作数是一条指令
			{
				instruction *ins = mi->src_op(0).instr(); //将原操作数的指令用basesuif的指令类表示
				if (ins->opcode() == io_ldc) //如果这是一个读常量的指令
				{
					immed val = ((in_ldc *) ins)->value(); //读取ldc指令的常量值；立即数只能是整形，浮点和变量
					if (val.is_symbol()) //如果这个立即数是一个变量符号
					{
						sym_node *sn = val.symbol();
						if (sn->is_proc()) //如果这个符号节点是过程节点
						{
							instr_list.push_back(mi); //将这条调用指令写入instr_list中
							defs->remove(n); //将n这个call指令中的定义点置0
						}
					}
				}
			}
		}

		if ((int) mi->opcode() == mo_jalr /*|| (int)mi->opcode() == mo_jr */) //如果是跳转链接指令
		{
			instr_list.push_back(mi);
			defs->remove(n);
		}
	}

	//delete the related instuction of the call instruction from the defs
	while (!instr_list.empty()) {
		machine_instr *mi_instr = instr_list.front(); //返回list中第一个元素的引用
		instr_list.pop_front(); //移除第一个元素

		bit_set_iter iter(defs);
		while (!iter.is_empty()) {
			int i = iter.step();
			machine_instr *mi = (machine_instr *) the_reach->catalog()->lookup(
					i);
			cfg_block *cb = find_block_contains_instr(spawn_path, mi); //mi指令包含在激发路径中的那个块中
			for (int i = 0; i < mi->num_srcs(); i++) {
				operand opd = mi->src_op(i);
				if (opd.is_symbol() || opd.is_reg()) {
					tree_instr *t_ins = search_latest_def_point_before_instr(
							opd, mi->parent(), cb, spawn_path);
					if (t_ins == 0)
						continue;

					machine_instr *def_instr = (machine_instr *) t_ins->instr();
					if (def_instr == mi_instr) {
						defs->remove(i);
						instr_list.push_back(mi);
						continue;
					}
				}
			}
		}
	}
}


