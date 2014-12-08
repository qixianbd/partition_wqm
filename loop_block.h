#ifndef LOOP_BLOCK_H
#define LOOP_BLOCK_H

#include "common.h"
#include "super_block.h"

extern FILE *stat_fp;
extern boolean statistics;

class loop_block : public super_block
{
private:
	/* loop_entry dom loop_end and loop_entry is succ of loop_end */
    cfg_node *loop_entry;           /*entry cfg node of loop super block*/
    cfg_node *loop_end;             /**/
    cfg_node_list *loop_exits;      /*cfg node list of loop super block*/
    super_block *likely_path;       /*the likely path(super block) of loop super block*/

    tnle *spawned_pos;              /*spawn position of loop block*/
    cfg_block *spawned_block;       /*the cfg block which spawn position is in*/
    tnle *cqip_pos;                 /*typedef tree_node_list_e tnle;  */
    cfg_block *cqip_block;          /**/
    tree_node_list *pslice;         /*pre-compute slice for loop*/
    label_sym *cqip_pos_num;        /*control quasi-independent point label*/
    cfg_node *succ_node;            /*loop region's successor node*/
    super_block *find_most_likely_path();

    bit_set *pslice_dept;

    /**
     * addbykeyming 20140516
     */
    label_sym *loop_label;
    static int loopid_counter;

public:
    loop_block();
    virtual ~loop_block();
    void set_entry_node(cfg_node *entry) {this->loop_entry = entry;}    /*set entry node of loop super block*/
    void set_end_node(cfg_node *end) {this->loop_end = end;}
    void add_exit_node(cfg_node *exitnode) {loop_exits->append(exitnode);}
    cfg_node* entry_node() {return loop_entry;}
    cfg_node* end_node() {return loop_end;}
    cfg_node_list* exit_nodes() {return loop_exits;}
    void set_cqip_pos(tnle *pos) {cqip_pos = pos;}
    tnle* get_cqip_pos() {return cqip_pos;}
    void set_spawned_pos(tnle *pos) {spawned_pos = pos;}
    tnle* get_spawned_pos() {return spawned_pos;}
    void set_cqip_block(cfg_block *block) {cqip_block = block;}
    cfg_block* get_cqip_block() {return cqip_block;}
    void set_spawned_block(cfg_block *block) {spawned_block = block;}
    cfg_block* get_spawned_block() {return spawned_block;}

    tree_node_list* get_pslice() {return pslice;}
    void set_pslice_dept(bit_set *instr_dept){pslice_dept = instr_dept;}
    bit_set *get_pslice_dept(){return pslice_dept;}
    bool is_loop_exit(cfg_node *);
    bool has_inner_loop();
    super_block* most_likely_path();
    virtual cfg_node* find_succ_node(da_cfg* the_cfg);//

    void set_spawn_info(tnle* spawn_pos, cfg_block* spawn_block,
                        tree_node_list* pslice, label_sym* cqip_pos_num);
    void write_spawn_info();

    /**
     * addbykeyming 20140515
     */
protected:
    label_sym* get_looplabel(){return loop_label;}
    void set_looplabel(label_sym* lplabel){loop_label = lplabel;}

    tnle* generate_loopbegin_pos();
    tnle* generate_loopend_pos();

public:
    void insert_loop_instrs();
    tnle* insert_loopbegin_instr();
    tnle* insert_loopend_instr();

};

/**
 * 将loop_likely_path中的所有节点的num用vec按序保存，然后返回
 * 返回值允许为empty
 *
 */
std::vector<int> get_num_vec_by_likely_path(super_block *loop_likely_path);

#endif
