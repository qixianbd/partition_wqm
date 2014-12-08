#ifndef ST_DA_CFG_H
#define ST_DA_CFG_H

#include <suif_copyright.h>
#include "common.h"
#include "super_block.h"
#include "loop_block.h"
#include "live_var_problem_more_syj.h"

extern void init_dfa(int &, char *[]);
extern void exit_dfa();

/*
 *class da_cfg
 *          control flow graph contains data analysis.
 */
class da_cfg : public cfg
{
private:
	live_var_problem_more_syj *liveness_more;
public:
	enum PrintGraphFlag{BEFORE = 0, AFTER = 1};
public:
    da_cfg(tree_block *b,
            boolean build_blocks = TRUE,
            boolean break_at_call = FALSE,
            boolean keep_layout = FALSE);

    ~da_cfg();

    void find_live_vars_more(operand_bit_manager *);

    boolean live_in(cfg_node *n, int var_num)
    {return liveness_more->live_in(n, var_num);}

    bit_set* live_in_set(cfg_node *n)
    {return liveness_more->live_in_set(n);}

    boolean live_out(cfg_node *n, int var_num)
    {return liveness_more->live_out(n, var_num);}

    bit_set *live_out_set(cfg_node *n)
    {return liveness_more->live_out_set(n);}

    /**
     * 此函数不推荐使用，all_def的概念本身不清.我使用def_set代替
     * by SYJ ,2012-03-07
     */
   /* bit_set *all_def_set(cfg_node *cn)
    {return liveness_more->all_def_set(cn);}*/

    /**
     * 获取cn上的def, by SYJ ,2012-03-07
     */
    bit_set *def_set(cfg_node *cn)
    {
    	return liveness_more->def_set(cn);
    }

    /**
     * 覆盖父类cfg中的实现
     */
    boolean is_loop_exit(int n){
    	return is_loop_exit(node(n));
    }

    /**
     * 覆盖父类cfg中的实现
     */
    boolean is_loop_exit(cfg_node *cn) {
		cfg_node_list_iter succ_iter(cn->succs());
		while (!succ_iter.is_empty()) {
			cfg_node *succ_cn = succ_iter.step();

			if (loop_depth(succ_cn) < loop_depth(cn))
				return TRUE;

			//if()
		}
		return FALSE;
	}

    bit_set *use_set(cfg_node *cn)
    {return liveness_more->use_set(cn);}

    void local_info_update(cfg_node *cn)
    { liveness_more->local_info_update(cn);}

    boolean single_block_update(cfg_node *cn)
    {return liveness_more->single_block_update(cn);}

    void add_block_update(cfg_node *cn)
    { liveness_more->add_block_update(cn);}

	/**
	 * 对all_use的使用需要谨慎，不要使用
	 */
   /* void subset_all_def(cfg_node_list *cnl, bit_set *alld)
    { liveness_more->subset_all_def(cnl, alld);}*/

	/**
	 * 对all_use的使用需要谨慎，不要使用
	 */
   /* void subset_all_use(cfg_node_list *cnl, bit_set *allu)
    { liveness_more->subset_all_use(cnl, allu);}*/

    void subset_repeated_def(cfg_node_list *cnl, bit_set *rdef)
    { liveness_more->subset_repeated_def(cnl, rdef);}

    void subset_liveness_update(cfg_node_list *cnl)
    { liveness_more->subset_liveness_update(cnl);}

    //find super block sb's live-ins, conserve in the bit_set livein.
    void find_liveins(super_block *sb, operand_bit_manager *bit_man, bit_set *livein);


    void find_liveins_and_defs(super_block *sb, operand_bit_manager *bit_man,
    		bit_set *livein, bit_set *def);

    /*
     * 为cn中从mi开始（包括mi）的后部分代码段计算live_in和def
     */
    void find_liveins_after_pos(super_block *sb, machine_instr *mi, operand_bit_manager *bit_man,
       		bit_set *livein);

    void find_liveins_and_def_after_pos(super_block *cqip_path, machine_instr *mi, operand_bit_manager *bit_man,
       		bit_set *livein, bit_set *def);
    /*
     * 为cn中从mi开始（包括mi）的后部分代码段计算live_in和def
     */
    void find_cn_liveins_and_defs_after_pos(cfg_node *cn, machine_instr *mi, operand_bit_manager *bit_man,
    		bit_set *livein, bit_set *def);





    void find_loop_liveins_at_cqip_pos(super_block *loop_path, machine_instr *mi, operand_bit_manager *bit_man,
       		bit_set *livein);

    //void find_liveins_of_loop(loop_block *sb, operand_bit_manager *bit_man, bit_set *livein);

    //print control flow graph and data flow analysis.
    void print_cfg(FILE *fp = stdout);
    void print_dfa(FILE *fp = stdout);

    void generate_dot(FILE *fp = stdout);
    void generate_dot(char *procedureName, PrintGraphFlag suffixFlag);
    void generate_full_instr_dot(FILE *fp = stdout);
};

#endif
