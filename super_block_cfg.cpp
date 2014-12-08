#include "super_block_cfg.h"
#include "da_cfg.h"
#include <algorithm>

extern std::vector<loop_block *>*loops; /*vector which contains loop super block */

/*
 *super_block_cfg::super_block_cfg() --- constructor of super block cfg
 */
super_block_cfg::super_block_cfg()
{
    sb_list = new super_block_list;
}

/*
 *super_block_cfg::append() --- append the super block block to the super block list
 */
void super_block_cfg::append(super_block *block)
{
    sb_list->append(block);
}

/*
 *super_block_cfg::in_which_super_block() --- find cfg_node(node) in which super block.
 */
super_block* super_block_cfg::in_which_super_block(cfg_node *node) const
{
    assert(node != 0);
    super_block_list_iter iter(sb_list);
    while(!iter.is_empty())
    {
        super_block *sb = iter.step();
        if(sb->contains(node))
            return sb;
    }
    return 0;
}

/*
 *super_block_cfg::in_which_super_block() --- find tnle(pos) in which super block.
 */
super_block* super_block_cfg::in_which_super_block(tnle* pos) const
{
    tree_instr *t_instr = (tree_instr *)pos->contents;
    machine_instr *mi = (machine_instr *)t_instr->instr();

    super_block_list_iter iter(sb_list);
    while(!iter.is_empty())
    {
        super_block *sblock = iter.step();
        if(sblock->contains(mi))
            return sblock;
    }
    assert(0);
    return 0;
}


cfg_node *super_block_cfg::in_which_cfg_node(tnle *pos) const {
	tree_instr *t_instr = (tree_instr *) pos->contents;
	machine_instr *mi = (machine_instr *) t_instr->instr();

	super_block_list_iter iter(sb_list);
	while (!iter.is_empty()) {
		super_block *sblock = iter.step();
		if (sblock->contains(mi))
			return sblock->in_which_block(pos);
	}assert(0);
	return 0;
}


super_block *super_block_cfg::get_super_block_by_num(const int num) const{
	assert(num >= 0);
	super_block *ret = NULL;
	super_block_list_iter iter(sb_list);
	while (!iter.is_empty()) {
		super_block *sb = iter.step();
		if (sb->block_num() == num) {
			ret = sb;
			break;
		}
	}
	return ret;
}


/*
 *super_block_cfg::postdominates() --- determine super block candidate_pdom whether is postdominator of super block(block).
 */
bool super_block_cfg::postdominates(super_block *candidate_pdom, super_block *block)
{
    super_block *pdom = block->immed_pdom();
    while(pdom != 0)
    {
        if(pdom == candidate_pdom)
            return true;

        pdom = pdom->immed_pdom();
    }

    return false;
}


/*
 *super_block_cfg::print_relationship() --- print super block cfg.
 */
void super_block_cfg::print_relationship(FILE *fp)
{
    super_block_list_iter iter(sb_list);
    while(!iter.is_empty())
    {
        super_block *sb = iter.step();
        sb->print_relationship(fp);
    }
}



//使用全局变量cfg_node_list *the_full_cnl
cfg_node *get_cfg_node_by_tnle(tnle *test) {
	assert(NULL != the_full_cnl);
	assert(NULL != test);

	cfg_node_list_iter iter(the_full_cnl);
	while (!iter.is_empty()) {
		cfg_node *node = iter.step();

		if (node->is_block()) {
			cfg_block *blk = (cfg_block*) node;
			if (blk->contains(test)) {
				return node;
			}
		}
	}

	return 0;
}


/**
 * addbykeyming 20140527-----------------------------------------------
 * ---------------------------------------------------------------------
 */

void
super_block_cfg::add_sp_block(int sp_num)
{
	std::vector<int>::iterator pos = find(sp_block_list.begin(), sp_block_list.end(), sp_num);
	if(pos == sp_block_list.end()){
		this->sp_block_list.push_back(sp_num);
	}
}

std::vector<int>&
super_block_cfg::get_sp_block_list()
{
	return this->sp_block_list;
}

void
super_block_cfg::add_cqip_block(int cqip_num)
{
	std::vector<int>::iterator pos = find(cqip_block_list.begin(), cqip_block_list.end(), cqip_num);
	if(pos == cqip_block_list.end()){
		this->cqip_block_list.push_back(cqip_num);
	}
}


std::vector<int>&
super_block_cfg::get_cqip_block()
{
	return this->cqip_block_list;
}


/**
 * Find the optional spawn numbers given by sp_num. Then insert the <sp_num, opt_sp_num_list> pair into
 * the attribute of opt_sp_num_map.
 * @param sp_num[in] the sp_block_num set in the first partition pass.
 * @[out],  this method will change the opt_sp_num_map's content.
 * @return[out] the numbers for the given sp_num, if there is none, return 0, others the return number > 0
 */


int
super_block_cfg::add_opt_sp_num_pair(int sp_num)
{

}


/**
 * the same as the add_opt_sp_num_pair above.
 * @param cqip_num
 * @return
 */
int
super_block_cfg::add_opt_cqip_num_pair(int cqip_num)
{

}

/**
 * 找到本节点的所有支配节点集合.
 */
void
super_block_cfg::find_dom_block_list()
{
	super_block *start_block = this->entry_block->fall_succ();
	super_block *end_block = this->exit_block;

	super_block *the_sblock = start_block;
	while(the_sblock != end_block){
		this->dom_block_list.push_back(the_sblock->block_num());
		the_sblock = the_sblock->immed_pdom();
	}
}

bool
super_block_cfg::is_in_loop(int sp_num)
{
	assert(the_cfg);
	if(the_cfg->loop_depth(sp_num) == 0){
		return false;
	}
	else{
		return true;
	}
}

bool
super_block_cfg::is_dominate(int sp_num)
{
	int size = this->dom_block_list.size();
	for(int i = 0; i < size; i++){
		if(sp_num == dom_block_list[i]){
			return true;
		}
	}
	return false;
}

void
super_block_cfg::fill_sp_attribute_map()
{
	if(!sp_block_list.empty()){
		for(int i = 0; i < sp_block_list.size(); i++){
			add_sp_attribute_pair(sp_block_list[i]);
		}
	}
	/**
	 * 下面的是断言, 为了部分的验证程序的正确性验证, 不含入主逻辑中.
	 */
	int sp_block_list_size = sp_block_list.size();
	int sp_attribute_map_size = sp_block_map.size();
	assert(sp_block_list_size == sp_attribute_map_size);

	return ;
}
void
super_block_cfg::add_sp_attribute_pair(int sp_num)
{
	sp_block_attribute sp_attr;
	//1. 判断是否是在循环内. 以及本身是否是支配节点.
	sp_attr.is_dom = is_dominate(sp_num);
	sp_attr.is_in_loop = is_in_loop(sp_num);

	// 2. 找到最近前向支配节点.
	int size = dom_block_list.size(), predom = 0, postdom = 0;
	for(int i = 0; i < size; i++){
		if(the_cfg->dominates(dom_block_list[i], sp_num)){
			predom = dom_block_list[i];
		}
		else{
			break;
		}
	}

	//3. 找到最近后向支配节点.
	for(int i = size -1; i >= 0; i--){
		if(the_cfg->postdominates(dom_block_list[i], sp_num)){
			postdom = dom_block_list[i];
		}
		else{
			break;
		}
	}
	sp_attr.pre_dom = predom;
	sp_attr.post_dom = postdom;

	//4. 添加到map中.
	std::map<int, sp_block_attribute>::iterator pos = this->sp_block_map.find(sp_num);
	if(pos == sp_block_map.end()){
		this->sp_block_map[sp_num] = sp_attr;
		/**
		 * 将predom 加入到sp_pdom_block_list中
		 */
		this->sp_post_dom_list.push_back(predom);
	}
	return ;
}

/**
 * --------------------helper functions ----------------------------------------------------------
 * -----------------------------------------------------------------------------------------------
 */
int
super_block_cfg::is_mutual_block(cfg_block *candidate_block, cfg_block *sp_block)
{

}

int
super_block_cfg::find_mutual_blocks(cfg_block *sp_block)
{
	//1.
}


/**
 * -------------------------------test functions-------------------------------------------
 * -----------------------------------------------------------------------------------------
 * test the functions wrote by keyming.
 */

void
super_block_cfg::print_dom_block_list(FILE *fp)
{
	//this->find_dom_block_list();
	int size = this->dom_block_list.size();
	for(int i = 0; i < size; i++){
		fprintf(fp, "%d \t", this->dom_block_list[i]);
	}
	fprintf(fp, "\n");
}

void
super_block_cfg::print_sp_and_cqip_block_list(FILE *fp)
{
	this->print_sp_block_list(fp);
	this->print_cqip_list(fp);
}

void
super_block_cfg::print_sp_block_list(FILE *fp)
{
	int size = sp_block_list.size();
	for(int i = 0; i < size; i++){
		fprintf(fp, "%d \t", sp_block_list[i]);
	}
	fprintf(fp, "\n");
}

void
super_block_cfg::print_cqip_list(FILE *fp)
{
	int size = cqip_block_list.size();
	for(int i = 0; i < size; i++){
		fprintf(fp, "%d \t", cqip_block_list[i]);
	}
	fprintf(fp, "\n");
}

bool super_block_cfg::is_in_sp_block_map(int block_num)const
{
	if(this->sp_block_map.find(block_num) != sp_block_map.end()){
		return true;
	}
	return false;
}

void
super_block_cfg::print_sp_attribute_map(FILE *fp)
{
	std::map<int, sp_block_attribute>::iterator iter = sp_block_map.begin();
	for(; iter != sp_block_map.end(); iter++){
		fprintf(fp, "sp_num %d\t:", iter->first);
		(iter->second).print(fp);
	}
}

