#include <suif_copyright.h>
#include "super_block.h"
#include "loop_block.h"
#include "super_block_cfg.h"
#include "da_cfg.h"

extern char *k_call_overhead;

/*
 *super_block::super_block() --- According to block kind, construct super block.
 */
super_block::super_block(super_block_kind block_kind) :
		kind(block_kind) {
	super_block_num = 0;
	cnl = new cfg_node_list();
	succs[0] = 0;
	succs[1] = 0;
	taken_probability = 0.00001;
	immed_dominator = 0;
	immed_pdominator = 0;
}

/*
 *super_block::~super_block() --- destructor of super block
 */
super_block::~super_block() {
	delete cnl;
}

/*
 *super_block::set_block_num() --- set each super block's number.
 */
void super_block::set_block_num(int num) {
	this->super_block_num = num;
}

/*
 *super_block::nodes() --- return super block's cfg node list.
 */
cfg_node_list *super_block::nodes() const {
	return cnl;
}

/*
 *super_block::size() --- compute super block's size(dynamic instruction number)
 */
int super_block::dynamic_size() {
	int size = 0;
	cfg_node_list_iter cnl_iter(cnl);
	while (!cnl_iter.is_empty()) {
		cfg_node *curr_node = cnl_iter.step();
		if (curr_node->is_block()) {
			size += block_dynamic_size((cfg_block *) curr_node);
		}
	}
	return size;
}

/*
 *super_block::instr_size() --- compute super block's instruction size, don't contain lab, loc, and null
 *return count of instr , region [1, count]
 */
int super_block::static_size() {
	int counter = 0;
	cfg_node_list_iter cnl_iter(cnl);
	while (!cnl_iter.is_empty()) {
		cfg_node *cn = cnl_iter.step();
		if (cn->is_block()) {
			cfg_node_instr_iter cnii(cn);
			while (!cnii.is_empty()) {
				tree_instr *test = cnii.step();
				machine_instr *mi = (machine_instr *) test->instr();
				if ((int) mi->opcode() == mo_lab || (int) mi->opcode() == mo_loc
						|| (int) mi->opcode() == mo_null)
					continue;
				else
					counter++;
			}
		}
	}
	return counter;
}

/*
 *super_block::add_block() --- add cfg node to cfg node list(cnl).
 */
void super_block::add_block(cfg_node * the_block) {
	if (the_block == NULL)
		fprintf(stderr, "add a NULL block\n");

	//If the block doesn't belong to nodes()
	if (!occurs(the_block, nodes())) {
		cnl->append(the_block);
	}
}

/*
 *super_block::add_blocks() --- add cfg node list to current super block
 */
void super_block::add_blocks(cfg_node_list * the_list) {
	cfg_node_list_iter the_list_iter(the_list);
	while (!the_list_iter.is_empty()) {
		cfg_node *cn = the_list_iter.step();
		if (!this->contains(cn)) {
			cnl->append(cn);
		}
	}
}

/*
 *super_block::first_block() --- find the first cfg block of the super block.
 *the_full_cnl -- is gloable variable in main.cpp
 */
cfg_node *super_block::first_block() {
	cfg_node *first = NULL;
	cfg_node_list_iter full_cnl_iter(the_full_cnl);
	while (!full_cnl_iter.is_empty()) {
		cfg_node *test = full_cnl_iter.step();
		if (occurs(test, nodes())) {
			first = test;
			break;
		}
	}
	return first;
}

/*
 *super_block::last_block() --- return the last block of the super block
 */
cfg_node *super_block::last_block() {
	cfg_node *last = NULL;
	cfg_node_list_iter full_cnl_iter(the_full_cnl);
	while (!full_cnl_iter.is_empty()) {
		cfg_node *test = full_cnl_iter.step();
		if (occurs(test, nodes())) {
			last = test;
		}
	}
	return last;
}

/*
 *super_block::instr_access() --- return the nth instruction of the super block.
 *start at 0.
 */
machine_instr *super_block::instr_access(int n) {
	tree_instr *test;
	int counter = 0;
	cfg_node_list_iter cnl_iter(cnl);
	while (!cnl_iter.is_empty()) {
		cfg_node *cn = cnl_iter.step();
		if (cn->is_block()) {
			cfg_node_instr_iter cnii(cn);
			while (!cnii.is_empty()) {
				test = cnii.step();
				machine_instr *mi = (machine_instr *) test->instr();
				if ((int) mi->opcode() == mo_null
						|| (int) mi->opcode() == mo_lab
						|| (int) mi->opcode() == mo_loc) {
					continue;
				} else if (counter == n) {
					return mi;
				} else {
					counter++;
				}
			}
		}
	}
	return 0;
}

/*
 tnle* super_block::instr_access(int n)
 {
 tree_instr *test;
 int counter = 0;
 cfg_node_list_iter cnli(cnl);
 while(!cnli.is_empty())
 {
 cfg_node *cn = cnli.step();
 if(cn->is_block())
 {
 cfg_node_instr_iter cnii(cn);
 while(!cnii.is_empty())
 {
 test = cnii.step();
 if(counter == n)
 return test->list_e();
 else
 counter++
 }
 }
 }
 return NULL;
 }
 */

/*
 *super_block::instr_access() --- return the n'th instruction of cfg node(node)
 *start at 0.
 */
machine_instr *super_block::instr_access(cfg_node * node, int n) {
	tree_instr *test;
	int counter = 0;
	cfg_node_list_iter cnl_iter(cnl);
	while (!cnl_iter.is_empty()) {
		cfg_node *cn = cnl_iter.step();
		if (cn == node) {
			cfg_node_instr_iter cnii(cn);
			while (!cnii.is_empty()) {
				test = cnii.step();
				machine_instr *mi = (machine_instr *) test->instr();
				if ((int) mi->opcode() == mo_null
						|| (int) mi->opcode() == mo_lab
						|| (int) mi->opcode() == mo_loc) {
					continue;
				} else if (counter == n) {
					return mi;
				} else {
					counter++;
				}
			}
		}
	}
	return 0;
}

/*
 *super_block::instr_count() --- return the machine instruction(mi)'s number ID
 *return index[0, (end-1)]
 */
int super_block::instr_count(machine_instr * mi) {
	int index = 0;
	cfg_node_list_iter cnl_iter(cnl);
	while (!cnl_iter.is_empty()) {
		cfg_node *cn = cnl_iter.step();
		if (cn->is_block()) {
			cfg_node_instr_iter cnii(cn);
			while (!cnii.is_empty()) {
				tree_instr *test = cnii.step();
				if ((machine_instr *) test->instr() == mi) {
					return index;
				} else if ((int) ((machine_instr *) test->instr())->opcode()
						== mo_null
						|| (int) ((machine_instr *) test->instr())->opcode()
								== mo_lab
						|| (int) ((machine_instr *) test->instr())->opcode()
								== mo_loc) {
					continue;
				} else {
					index++;
				}
			}
		}
	}
	return 0;
}

/*
 *super_block::contains() --- determine the super block whether contains the cfg node(node).
 */
bool super_block::contains(cfg_node * node) {
	cfg_node_list_iter iter(cnl);
	while (!iter.is_empty()) {
		cfg_node *mynode = iter.step();
		if (mynode == node)
			return true;
	}

	return false;
}

/*
 *super_block::contains() --- determine the super block whether contains machine instruction(mi).
 */
bool super_block::contains(machine_instr * mi) {
	cfg_node_list_iter cnli(cnl);
	while (!cnli.is_empty()) {
		cfg_node *cn = cnli.step();
		if (cn->is_block()) {
			cfg_block *block = (cfg_block *) cn;
			cfg_node_instr_iter cnii(block);
			while (!cnii.is_empty()) {
				machine_instr *mi_in_block =
						(machine_instr *) (cnii.step())->instr();
				if (mi == mi_in_block)
					return true;
			}
		}
	}
	return false;
}

/*
 *super_block::in_which_block() --- find tnle position in which block, and return the cfg_block
 */
cfg_block *super_block::in_which_block(tnle * pos) {
	cfg_node_list_iter iter(cnl);
	while (!iter.is_empty()) {
		cfg_node *node = iter.step();
		if (node->is_block()) {
			cfg_block *block = (cfg_block *) node;
			if (block->contains(pos))
				return block;
		}
	}
	return 0;
}

/*
 *super_block::set_immed_dom() --- set current super block's immed dominator.
 */
void super_block::set_immed_dom(super_block * immed_dom) {
	this->immed_dominator = immed_dom;
}

/*
 *super_block::immed_dom() --- return current super block's immed dominator.
 */
super_block *super_block::immed_dom() {
	return this->immed_dominator;
}

/*
 *super_block::set_immed_pdom() --- set current block's immed postdominator.
 */
void super_block::set_immed_pdom(super_block * immed_pdom) {
	this->immed_pdominator = immed_pdom;
}

/*
 *super_block::immed_pdom() --- return current super block's immed postdominator.
 */
super_block *super_block::immed_pdom() {
	return this->immed_pdominator;
}

/*
 *super_block::set_fall_succ() --- set current super block's fall successor.
 */
void super_block::set_fall_succ(super_block * fall_succ) {
	succs[0] = fall_succ;
}

/*
 *super_block::fall_succ() --- return fall successor of the super block.
 */
super_block *super_block::fall_succ() {
	return succs[0];
}

/*
 *super_block::set_take_succ() --- set current super block's take successor.
 */
void super_block::set_take_succ(super_block * take_succ, double taken_prob) {
	succs[1] = take_succ;
	this->taken_probability = taken_prob;
}

/*
 *super_block::take_succ() --- return current super block's take successor.
 */
super_block *super_block::take_succ() {
	return succs[1];
}

/*
 *super_block::likely_succ() --- return current super block's likely successor.
 */
super_block *super_block::likely_succ() {
	if (taken_prob() > 0.5)
		return take_succ();
	else
		return fall_succ();
}


super_block *super_block::less_likely_succ(){
	if(taken_prob() < 0.5)
		return take_succ();
	else
		return fall_succ();
}

/*
 *super_block::taken_prob() --- return current super block's taken probability.
 */
double super_block::taken_prob() {
	return taken_probability;
}

/*
 *super_block::flush() --- flush current cfg node list.
 */
void super_block::flush() {
	delete cnl;
	cnl = new cfg_node_list();
}

/*
 *super_block::print() --- print super block
 */
void super_block::print(FILE * fp) {
	cfg_node_list_iter cnli(cnl);
	while (!cnli.is_empty()) {
		cfg_node *node = cnli.step();
		if (node == NULL) {
			fprintf(stderr, "access NULL\n");
			exit(1);
		}
		node->print(fp);
	}
}

/*
 *super_block::print_relationship() --- print super block relationship, such as: cfg nodes contained in each super block, and
 *                                      each super block's fall super block, take super block, dominator, and postdominator
 *                                      super block.
 */
void super_block::print_relationship(FILE * fp) {
	fprintf(fp, "super block %d : ", super_block_num);
	fprintf(fp, "contain: <");
	cfg_node_list_iter iter(cnl);
	while (!iter.is_empty()) {
		cfg_node *node = iter.step();
		fprintf(fp, "%d ", node->number());
	}
	fprintf(fp, ">");

	fprintf(fp, "fall:");
	if (succs[0] == 0)
		fprintf(fp, "nil  ");
	else
		fprintf(fp, "%d   ", succs[0]->block_num());

	fprintf(fp, "take:");
	if (succs[1] == 0)
		fprintf(fp, "nil ");
	else
		fprintf(fp, "%d  ", succs[1]->block_num());

	fprintf(fp, "dom:");
	if (immed_dominator == 0)
		fprintf(fp, " nil  ");
	else
		fprintf(fp, "%d  ", immed_dominator->block_num());

	fprintf(fp, "pdom:");
	if (immed_pdominator == 0)
		fprintf(fp, " nil ");
	else
		fprintf(fp, "%d   ", immed_pdominator->block_num());
	fprintf(fp, "\n");
}



super_block *super_block::get_nloop_spawn_super_block_by_vec(
		const std::vector<int> & spawn_vec, da_cfg * cfg) {
	assert(!spawn_vec.empty());

	super_block *nodes = new super_block(super_block::BLOCK_LIST);
	for (int i = 0; i < spawn_vec.size(); ++i) {
		cfg_node *cn = cfg->node(spawn_vec[i]);

		nodes->add_block(cn);
		assert(! cfg->is_loop_begin(cn));//此处不必加限制if(i > 0)
	}
	return nodes;
}

//have some problem
/*
 *super_block::operator=() --- super block's copy constructor 
 */
super_block & super_block::operator=(const super_block & other) {
	if (this == &other)
		return *this;
	delete cnl;

	cnl = new cfg_node_list();
	cnl->copy(other.cnl);
	return *this;
}






//-------------------------------------------------------------------

int cfg_node_static_instr_size(cfg_node *node) {
	assert(node->is_block());

	int counter = 0;
	cfg_node_instr_iter cnii(node);
	while (!cnii.is_empty()) {
		tree_instr *test = cnii.step();
		machine_instr *mi = (machine_instr *) test->instr();
		if ((int) mi->opcode() == mo_lab || (int) mi->opcode() == mo_loc
				|| (int) mi->opcode() == mo_null){
			continue;
		}
		else{
			counter++;
		}
	}

	return counter;
}

machine_instr *instr_access(cfg_node *node, int index) {
	assert(NULL != node);
	assert(index >= 0);
	assert(node->is_block());
	int count = cfg_node_static_instr_size(node);
	assert(index < count);

	machine_instr *target = NULL;
	cfg_node_instr_iter instrs(node);
	while (!instrs.is_empty()) {
		tree_instr *ti = instrs.step();
		if ((int) ((machine_instr *) ti->instr())->opcode() == mo_null
				|| (int) ((machine_instr *) ti->instr())->opcode() == mo_lab
				|| (int) ((machine_instr *) ti->instr())->opcode() == mo_loc) {
			continue;
		} else if (index == 0) {
			target = (machine_instr*)ti->instr();
			break;
		} else {
			--index;
		}
	}

	assert(NULL != target);

	return target;
}

/**
 * 动态指令数，包含函数调用代价,体现过程间关系
 */
int block_dynamic_size(cfg_block *cb){
	assert(cb->is_block());

	int size = 0;
	cfg_node_instr_iter instr_iter(cb);
	while (!instr_iter.is_empty()) {
		tree_instr *ti = instr_iter.step();
		machine_instr *mi = (machine_instr *) ti->instr();
		if ((int) mi->opcode() == mo_null || (int) mi->opcode() == mo_lab
				|| (int) mi->opcode() == mo_loc) {
			continue;
		} else if ((int) mi->opcode() == mo_jalr) {
			immed_list *iml = (immed_list *) (mi->peek_annote(k_call_overhead));
			if (iml) {
				int overhead = (*iml)[0].integer();
				if (overhead == 0)
					size++;
				else
					size += overhead;
			} else
				size++;
		} else {
			size++;
		}
	}
	return size;
}


/*
 *block_dynamic_size_from_instr() --- compute block's size from the instruction.
 计算从这条指令开始的块的大小,不包括mo_null、mo_lab和mo_loc
  包括 函数调用代价-它体现了过程之间的关系
  当mi_instr本身是mo_jalr型指令
 */
int block_dynamic_size_from_instr(cfg_block * cb, machine_instr * mi_instr) {
	int size = 0;
	bool is_begin = false;
	cfg_node_instr_iter cb_instr_iter(cb);
	while (!cb_instr_iter.is_empty()) {
		tree_instr *ti = cb_instr_iter.step();
		machine_instr *mi = (machine_instr *) ti->instr();
		//从mi_instr指令算起，它是块中的第一条指令
		if (mi == mi_instr) {
			is_begin = true;
		}

		if (is_begin) {
			if ((int) mi->opcode() == mo_null || (int) mi->opcode() == mo_lab
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

	assert(size != 0);
	return size;
}



super_block* super_block::multi_likely_succ() {
	if (taken_prob() < 0.15)
		return fall_succ();
	else
		return take_succ();

}

bool super_block::taken_multi_suit() {
	if(taken_prob() >= 0.001){
		return true;
	}
	return false;
}

bool super_block::fall_multi_suit() {
	if(taken_prob() <= 0.999){
		return true;
	}
	return false;
}
