#ifndef SUPER_BLOCK_CFG
#define SUPER_BLOCK_CFG

#include "common.h"
#include "super_block.h"
#include <map>
#include <utility>

class spawn_block
{
public:
	int block_num;
	std::pair<int, int> dominate_pair;
public:
	bool is_dominate_node(super_block_cfg *proc_cfg);
	void find_the_dom_pdom_pair(super_block_cfg *proc_cfg);
};

/**
 * addbykeyming 20140529----------------------------------------------------
 * sp_block_attribute 主要用来存储对于给定sp_block的前后支配节点[pre_dom, post_dom].
 */
struct sp_block_attribute
{
	int pre_dom;		// the pre dominator ( the dominator in the functions whole cfg. ie from begin to end) of the sp_block.
						//如果一个节点的pre_dom = 0, 对应的是begin node.
	int post_dom;		// 该sp_block节点紧挨着的下一个支配节点. (该支配节点是属于整个函数的支配节点集合, 也就是dom_block_list的).
						//如果一个节点的post_num = 0, 表明, 其下一个支配节点为end node.
	bool is_dom;		// 该节点本身是否是支配节点
	bool is_in_loop;	//该节点 是否在循环内部
	sp_block_attribute():pre_dom(0), post_dom(0), is_dom(false), is_in_loop(false){}
	sp_block_attribute(int pre, int post, bool isdom, bool inloop):pre_dom(pre),
			post_dom(post), is_dom(isdom), is_in_loop(inloop)
	{

	}
	void print(FILE *fp = stdout)
	{
		fprintf(fp, "%d\t%d\t%d\t%d\n", pre_dom, post_dom, (int)is_dom, (int)is_in_loop);
	}
};


class super_block_cfg
{
private:
    super_block_list *sb_list;      /*super block list contains super block*/
    super_block *entry_block;       /*entry super block of super block list*/
    super_block *exit_block;        /*exit super block of super block list*/

    /**
     * addbykeyming 20140527
     */
    std::vector<int> dom_block_list;     //本函数的所有支配节点集合.
    std::vector<int> sp_block_list; 		// 本函数(当前已经)包含的划分点数组. 用以指导第二遍划分.
    std::vector<int> cqip_block_list;    // 本函数(当前已经)包含的cqip点数组, 用以指导第二编划分.

    std::map<int, sp_block_attribute> sp_block_map;			//sp_block 到sp_attibute的映射.
    std::vector<int> sp_post_dom_list;						//sp_block的pdom 链表


    // below not used.
    std::map<int, std::vector<int> > opt_sp_num_map; //<sp_num, opt_sp_num_list>第二遍划分时, 当前函数对应某个sp点的opt-spawn-block
    std::map<int, std::vector<int> > opt_cqip_num_map; //<cqip_num, opt_cqip_num_list> 第二遍划分时, cqip_num 与 opt-cqip-block list 的映射



public:
    super_block_cfg();
    void append(super_block *block);
    super_block* entry() const {return entry_block;}
    super_block* exit() const {return exit_block;}
    void set_entry(super_block *block) {this->entry_block = block;}
    void set_exit(super_block *block) {this->exit_block = block;}
    super_block_list* super_blocks() {return sb_list;}
    super_block* in_which_super_block(cfg_node *node) const;
    super_block* in_which_super_block(tnle* pos) const;
    cfg_node*    in_which_cfg_node(tnle* pos) const;
    super_block* get_super_block_by_num(const int num)const;

    bool postdominates(super_block* candidate_pdom, super_block* block);

    void print_relationship(FILE *fp = stdout);

    /**
     * addbykeyming 20140527-----------------------------------------------
     * ---------------------------------------------------------------------
     */
    /**
     * 将block 号为sp_num 的block(这个block是插入的sp 所在的block)加入到sp_block_num数组里.
     * @param sp_num   插入的spawn instruction 所在的block.
     */
    void add_sp_block(int sp_num);
    /**
     * 返回sp_block_num
     */
    std::vector<int>& get_sp_block_list();

    /**
     * 将block 号为num 的block(这个block是插入的cqip 所在的block)加入到cqip_block_num数组里.
     * @param num   插入的cqip instruction 所在的block.
     */
    void add_cqip_block(int cqip_num);
    /**
     *
     * @return cqip_block_num
     */
    std::vector<int>& get_cqip_block();


    /**
     * Find the optional spawn numbers given by sp_num. Then insert the <sp_num, opt_sp_num_list> pair into
     * the attribute of opt_sp_num_map.
     * @param sp_num[in] the sp_block_num set in the first partition pass.
     * @[out],  this method will change the opt_sp_num_map's content.
     * @return[out] the numbers for the given sp_num, if there is none, return 0, others the return number > 0
     */
    int add_opt_sp_num_pair(int sp_num);


    /**
     * the same as the add_opt_sp_num_pair above.
     * @param cqip_num
     * @return
     */
    int add_opt_cqip_num_pair(int cqip_num);

    /**
     * 找到本节点的所有支配节点集合.
     */
    void find_dom_block_list();
    std::vector<int>& get_dom_block_list(){return dom_block_list;}

    bool is_in_loop(int sp_num);
    bool is_dominate(int sp_num);

    void add_sp_attribute_pair(int sp_num);
    void fill_sp_attribute_map();
    bool is_in_sp_block_map(int block_num)const;



    /**
     * --------------------helper functions ---------------------
     */
    int is_mutual_block(cfg_block *candidate_block, cfg_block *sp_block);
    int find_mutual_blocks(cfg_block *sp_block);

    /**
     * -----------------------test or print functions ---------------------------
     */
    void print_dom_block_list(FILE *fp = stderr);
    void print_sp_and_cqip_block_list(FILE *fp = stderr);
    void print_sp_block_list(FILE *fp = stderr);
    void print_cqip_list(FILE *fp);
    void print_sp_attribute_map(FILE *fp = stderr);

	const std::vector<int>& get_sp_post_dom_list() const {
		return sp_post_dom_list;
	}
};


//使用全局变量cfg_node_list *the_full_cnl
cfg_node *get_cfg_node_by_tnle(tnle *test);

#endif
