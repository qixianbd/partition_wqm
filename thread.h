#ifndef THREAD_H
#define THREAD_H

#include "super_block.h"
#include "super_block_path.h"
#include "node.h"    //for util::node *pNode
#include <vector>
extern FILE *stat_fp;
extern boolean statistics;

class thread : public super_block_path
{
public:
    enum thread_type{
        SPECULATIVE,
        NONSPECULATIVE,
        LOOP
    };

private:
    int s_count;                            /*spawn count*/
    tnle* spawned_pos;                      /*spawn position*/
    cfg_block* spawned_block;               /*cfg block which contains spawn position*/
    tree_node_list* pslice;                 /*pre-compute slice*/
    label_sym *cqip_pos_num;                /*control quasi-independent point*/
    cfg_block* cqip_block;                  /*cfg block which contains cqip position*/
    tnle* cqip_pos;                         /*cqip position*/
    std::vector<tnle *> spawn_poss;         /*vector of thread's spawn position*/
    thread_type thread_kind;                /*current thread type(speculative or nonspeculative)*/

public:
    thread();
    ~thread();
    void set_spawn_count(int n);
    int spawn_count();
    void inc_spawn_count();
    void set_spawn_info(cfg_block *spawn_block, tree_node_list *pslice,
                        label_sym *cqip_pos_num);
    void set_spawned_pos(tnle *spawn_pos);
    tnle* get_spawned_pos();

    void set_thread_type(thread_type type) {thread_kind = type;};
    thread_type get_thread_type() {return thread_kind;}

    void set_cqip_pos(tnle *cqip_pos);
    tnle* get_cqip_pos();

    void set_spawned_block(cfg_block *cb){spawned_block = cb;}
    cfg_block* get_spawned_block();
    void set_cqip_block(cfg_block *cb) {cqip_block = cb;}
    cfg_block* get_cqip_block();

    tree_node_list* get_pslice() {return pslice;}
    void add_spawn_pos(tnle *spawn_pos);

    bool reach_any_spawn_pos(tnle *spawn_pos);

    void print_block_list_num(FILE *fp);

    /**
     * @description construct a thread, which contains all the super_blocks in pNode and has setted spawn_pos
     */
    static thread* construct_thread_by_node_group(util::node *pNode,  const super_block_cfg *scfg);

    static thread* construct_loop_body_thread_by_group(util::node *pNode,  da_cfg *cfg);

    void write_spawn_info();
};
#endif
