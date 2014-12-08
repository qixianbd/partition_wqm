#ifndef SPAWN_POS_TRACE_H
#define SPAWN_POS_TRACE_H

#include <vector>
#include "common.h"
#include "super_block.h"
#include "super_block_cfg.h"
#include "thread.h"

class spawn_pos_trace: public super_block {
private:
	std::vector<machine_instr *> *related_cals; /*record the call instruction in the spawn position trace*/

public:
	spawn_pos_trace();
	~spawn_pos_trace();
	int related_cal_num();
	std::vector<machine_instr *>* get_related_cals();
	void analyze_related_cals(reaching_def_problem *the_reach,
			bit_set *related);

	void construct_loop_spawn_pos_trace(super_block *likely_path);

	void construct_spawn_pos_trace(super_block_cfg *scfg, thread *curr_thread,
			super_block *cqip_block);

	/**
	 * for min_cut
	 * 在sub_vec所表示路径上构造
	 */
	void construct_spawn_pos_trace(const std::vector<int> &sub_vec,
			da_cfg *cfg);
};

#endif
