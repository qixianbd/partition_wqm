#ifndef SUPER_BLOCK_H
#define SUPER_BLOCK_H

#include <suif_copyright.h>

#include "common.h"
//#include "super_block_cfg.h"
#include <vector>

class super_block_cfg;

class super_block
{
public:
    enum super_block_kind
    {
        BLOCK,
        BLOCK_LIST,
        LOOP,
        CFG_BEGIN,
        CFG_END
    };
private:
    int super_block_num;                /*super block's number ID*/
    super_block_kind kind;              /*super block's kind*/
    cfg_node_list *cnl;                 /*cfg node list*/
    super_block *succs[2];              /*super block's successors, contain fall and take successor*/
    double taken_probability;            /*successor taken probability*/
    super_block *immed_dominator;       /*super block's immed dominator*/
    super_block *immed_pdominator;      /*super block's immed postdominator*/

public:
    super_block(super_block_kind block_kind = BLOCK);
    ~super_block();
    super_block_kind knd(){return kind;}
    bool is_loop()const{return this->kind == LOOP;}
    void set_block_num(int num);
    int block_num(){return super_block_num;}
    cfg_node_list *nodes() const;
    int dynamic_size();
    int static_size();
    void add_block(cfg_node *);
    void add_blocks(cfg_node_list *);
    cfg_node *first_block();
    cfg_node *last_block();
    machine_instr *instr_access(int n);
//    tnle* instr_access(int n);
    machine_instr *instr_access(cfg_node *node, int n);
    int instr_count(machine_instr *);
    bool contains(cfg_node *node);
    bool contains(machine_instr *mi);
    cfg_block* in_which_block(tnle *pos);
    void set_immed_dom(super_block *immed_dom);
    super_block* immed_dom();
    void set_immed_pdom(super_block *immed_pdom);
    super_block* immed_pdom();
    void set_fall_succ(super_block *fall_succ);
    super_block* fall_succ();
    void set_take_succ(super_block *take_succ, double taken_prob);

    super_block* likely_succ();
    super_block* less_likely_succ();
    super_block* take_succ();
    double taken_prob();

    static super_block *get_nloop_spawn_super_block_by_vec(const std::vector<int> &spawn_vec, da_cfg * ccfg);

    void flush();
    void print(FILE *fp = stdout);
    void print_relationship(FILE *fp = stdout);
    super_block& operator=(const super_block &other);

    /**addbykeyming 20140623
     *
     */
    super_block* multi_likely_succ();
    bool taken_multi_suit();
    bool fall_multi_suit();

};

DECLARE_LIST_CLASS(super_block_list, super_block *);


/**
 * 动态指令数，包含函数调用代价
 */
int block_dynamic_size(cfg_block *cb);

/*
 *block_dynamic_size_from_instr() --- compute block's size from the instruction.
 计算从这条指令开始的块的大小,不包括mo_null、mo_lab和mo_loc
 包括 函数调用代价
 */
int block_dynamic_size_from_instr(cfg_block *cb, machine_instr *mi);
int cfg_node_static_instr_size(cfg_node *node);
machine_instr *instr_access(cfg_node *node, int index);

#endif
