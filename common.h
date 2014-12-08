#include <suif1.h>
#include <useful.h>
#include <machine.h>
#include <cfg.h>
#include <dfa.h>


class da_cfg;
class super_block_cfg;
extern operand_bit_manager *the_bit_mgr;
extern reg_def_teller *the_teller;  /*regiter define teller*/
extern reaching_def_problem *the_reach;
extern cfg_node_list *the_full_cnl;
extern da_cfg *the_cfg;
extern super_block_cfg *the_scfg; /*super block cfg composed of super block */
extern mproc_symtab *cur_psymtab;

extern void debug(const int lvl, const char *msg ...);

extern char *k_cqip_pos;
extern char *k_proc_profile;
extern char *k_instr_profile;
extern char *k_pslice_entry_pos;
extern char *k_pslice_exit_pos;
extern char *k_call_overhead;
extern char *k_looppos;   /* the begin of a loop*/
