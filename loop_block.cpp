#include "loop_block.h"
#include "spmt_instr.h"
#include "misc.h"
#include "XmlElement.h"
#include "da_cfg.h"
#include "common.h"

extern boolean usexml;
extern XmlElement *pslice_element;
extern int pslice_of_loop;

int loop_block::loopid_counter = 0;
/*
 *loop_block::loop_block() --- construct of loop super block.
 */
loop_block::loop_block() :
		super_block(super_block::LOOP) {
	loop_exits = new cfg_node_list;
	likely_path = 0;
	succ_node = 0;
}

/*
 *loop_block::~loop_block() --- destructor of loop block
 */
loop_block::~loop_block() {
	delete loop_exits;
	if (likely_path != 0)
		delete likely_path;
	//1.10
	if (succ_node != 0)
		delete succ_node;
}

/*
 *loop_block::is_loop_exit() --- determine cfg node whether is loop exit node.

 */
bool loop_block::is_loop_exit(cfg_node *node) {
	cfg_node_list_iter iter(loop_exits);
	while (!iter.is_empty()) //is_empty()用来测试一个循环的退出条件
	{
		cfg_node *exit_node = iter.step();
		if (exit_node == node)
			return true;
	}
	return false;
}

/*
 *
 */
bool loop_block::has_inner_loop() {
	bool bRet = false;

	const int loop_end_depth = the_cfg->loop_depth(loop_end);

	cfg_node_list_iter cnli(this->nodes());
	while (!cnli.is_empty()) {
		cfg_node *step_cn = cnli.step();
		//处理内层循环
		if (the_cfg->is_loop_begin(step_cn)
				&& the_cfg->loop_depth(step_cn) == loop_end_depth + 1) {
			bRet = true;
		}
	}

	return bRet;
}

/*
 *loop_block::most_likely_path() --- find the most likely path of the loop super block.
 */
super_block* loop_block::most_likely_path() {
	if (likely_path == 0)
		likely_path = find_most_likely_path();
	return likely_path;
}

/*
 *loop_block::find_most_likely_path() --- find the most likely path in loop region
 *找循环区域的最可能路径,
 *注意当含有内层循环时，
 */
super_block* loop_block::find_most_likely_path() {
	super_block *path = new super_block(super_block::BLOCK_LIST);
	cfg_node *node = this->loop_entry; //this指的是一个loop_block对象
	cfg_node *peek_node = NULL;
	//如果loop_block中的最可能的基本块包含进path并且没有再次到达loop_entry块
	do {
		path->add_block(node);
		if (branch_taken_probability(node) < 0.5) {
			node = node->fall_succ();
		} else {
			peek_node = node->take_succ();
			/**
			 * 避免碰到内层循环的回边时，出现死循环
			 * 仅当在prob为take，并且 path->contains(peek_node) == true时,出现内层循环
			 */
			if (path->contains(peek_node)) { //死循环
				node = node->fall_succ();
			} else {
				node = peek_node;
			}
		}
	} while (node != loop_entry && this->contains(node));

	return path;
}





/*
 *loop_block::find_succ_node() --- find succ_node of loop region
 找循环区域的后继节点
 */
cfg_node* loop_block::find_succ_node(da_cfg *the_cfg) {
	if (succ_node != 0)
		return succ_node;

	cfg_node *succ = 0;
	cfg_node_list_iter iter(loop_exits);
	while (!iter.is_empty()) {
		cfg_node *exit_node = iter.step();
		cfg_node_list_iter succ_iter(exit_node->succs());
		while (!succ_iter.is_empty()) {
			cfg_node *succ_node = succ_iter.step();
			if (!this->contains(succ_node)) {
				if (succ == 0) {
					succ = succ_node;
					continue;
				}
				/* else
				 {
				 //assert(succ == succ_node);
				 return NULL;
				 }*/
			}
		}
	}
	succ_node = succ;
	return succ_node;
}

/*
 *loop_block::set_spawn_info() --- set spawn information, contains spawn position, spawn block, pre-compute slice and cqip position label.
 */
void loop_block::set_spawn_info(tnle* spawn_pos, cfg_block* spawn_block,
		tree_node_list* pslice, label_sym* cqip_pos_num/*, tnle* cqip_pos*/) {
	this->spawned_pos = spawn_pos;
	this->spawned_block = spawn_block;
	this->pslice = pslice;
	this->cqip_pos_num = cqip_pos_num;
	//
//    this->cqip_pos = cqip_pos;
}

/*
 *loop_block::write_spawn_info() --- write spawn information, contains pre-compute slice, spawn instruction and cancel instruction. 
 */
void loop_block::write_spawn_info() {
//    spawned_block->insert_after(pslice, spawned_pos);
	//
	if (spawned_pos == 0 || cqip_pos == 0)
		assert_msg(FALSE, ("loop partition cqip position is null"));
	//if(cqip_pos->next())
	if (statistics)
		fprintf(stat_fp, "pslice size: \t%d\t\t", pslice->count());

	if (usexml) {
		pslice_element = new XmlElement("pslice_size", pslice->count());
		pslice_of_loop += pslice->count();
	}

	/*  printf("loop pslice:\n");*/
	tree_node_list_iter iter(pslice);
	while (!iter.is_empty()) {
		tree_node *tn = iter.step();
		((instruction *)((tree_instr *)tn)->instr())->print();
	}
	// printf("\n");
	cqip_block->insert_after(pslice, cqip_pos->next());
	//cqip_block->insert_after(pslice, cqip_pos->prev());

	//else
	//    assert_msg(FALSE,("loop partition doesn't insert pslice"));
	insert_spawn_instr(spawned_block, spawned_pos, cqip_pos_num);
	/*  printf("loop cqip:\n");
	 ((instruction *)((tree_instr *)(this->cqip_pos)->contents)->instr())->print();
	 printf("\n");*/

	/* printf("loop spawn:\n");
	 ((instruction *)((tree_instr *)(this->spawned_pos)->contents)->instr())->print();
	 printf("\n");*/

	/* printf("loop label:\n");
	 cqip_pos_num->print(stdout);
	 printf("\n");
	 printf("loop_block:\n");
	 this->print(stdout);
	 printf("\n");
	 printf("loop likely_path:\n");
	 this->likely_path->print(stdout);
	 printf("\n");
	 printf("loop_exits:\n");*/
	cfg_node_list_iter iter1(loop_exits);
	while (!iter1.is_empty()) {
		cfg_node *exit_node = iter1.step();
		/* exit_node->print(stdout);
		 printf("+++++");*/
		cfg_node_list_iter succ_iter(exit_node->succs());
		while (!succ_iter.is_empty()) {
			cfg_node *succ_node = succ_iter.step();
			/* succ_node->print(stdout);
			 printf("-----");*/
		}
		/* printf("\n");*/
	}


	//for loop, don't insert squash instruction
	//insert_cancel_instr(succ_node, cqip_pos_num);
}



//-----------------------------------------------------------------------

std::vector<int> get_num_vec_by_likely_path(super_block *loop_likely_path){
	assert(NULL != loop_likely_path);

	std::vector<int> num_vec;
	cfg_node_list *cnl = loop_likely_path->nodes();

	cfg_node_list_iter iter(cnl);
	while(!iter.is_empty()){
		cfg_node *cn = iter.step();
		num_vec.push_back(cn->number());
	}

	return num_vec;
}


/**
 * ----------------------addbykeyming--------------------------------------------------------
 * ----------------------addbykeyming--------------------------------------------------------
 * The following functions or methods are add by keyming for the purpose of multiple spawn.
 */

void loop_block::insert_loop_instrs()
{
	insert_loopbegin_instr();
	insert_loopend_instr();
}

tnle* loop_block::generate_loopend_pos()
{
	cfg_block *loopend_block = ((cfg_block*)this->loop_end);
	assert(loopend_block);
	tnle *loopend_pos = loopend_block->in_tail()->list_e();
	return loopend_pos;
	/*
	tnle pos(loopend_block);


	cfg_node_instr_iter cnii(loopend_block);
	while(!cnii.is_empty()){
		machine_instr *mi_in_block = (machine_instr *)(cnii.step()->instr());

	}
	*/
}


tnle* loop_block::generate_loopbegin_pos()
{
	cfg_block *loopbegin_block = ((cfg_block*)this->loop_entry);
	assert(loopbegin_block);
	return loopbegin_block->first_non_label();
}


tnle* loop_block::insert_loopbegin_instr()
{
	set_looplabel(cur_psymtab->pst()->new_unique_label());
	mi_lab *looplabel_mi = new mi_lab(mo_lab, loop_label);

	immed_list *an_iml = new immed_list;
	an_iml->append(immed(loop_label));
	loopid_counter++;

	tnle *loopbegin_pos = generate_loopbegin_pos();

	machine_instr *loopbegin_mi = new mi_bj(mo_loopbegin, loop_label);

	loopbegin_mi->print(stderr);
	loopbegin_mi ->set_annote(k_looppos, an_iml);


	char * opstring = loopbegin_mi->op_string();
	tree_node_list_e *loopbegin_tnle = new tree_node_list_e(new tree_instr(loopbegin_mi));

	((cfg_block*)this->loop_entry)->insert_after(loopbegin_tnle, loopbegin_pos);
	opstring = loopbegin_mi->op_string();
	tree_node_list_e *looplabel_tnle = new tree_node_list_e(new tree_instr(looplabel_mi));
	((cfg_block*)this->loop_entry)->insert_after(looplabel_tnle, loopbegin_tnle);
	return loopbegin_tnle;
}

tnle* loop_block::insert_loopend_instr()
{
	immed_list *an_iml = new immed_list;
	an_iml->append(immed(loop_label));

	tnle *loopend_pos = generate_loopend_pos();
	machine_instr *loopend_mi = new mi_bj(mo_loopend, loop_label);

	loopend_mi->print(stderr);
	char * opstring = loopend_mi->op_string();

	loopend_mi->set_annote(k_looppos, an_iml);

	tree_node_list_e *loopend_tnle = new tree_node_list_e(new tree_instr(loopend_mi));

	((cfg_block*)this->loop_end)->insert_after(loopend_tnle, loopend_pos);
}





