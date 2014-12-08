#include "common.h"
#include "misc.h"
#include "super_block.h"
#include "super_block_cfg.h"

#include <string.h>

/*
 *print_bit_man() --- print the bit manager.
 */
void print_bit_man(operand_bit_manager *the_bit_mgr, FILE *fp) {
	int n = the_bit_mgr->num_bits();
	bit_set *bs = new bit_set(0, n - 1);
	for (int i = 0; i < n; i++) {
		bs->clear();
		bs->add(i);
		bs->print(fp);
		the_bit_mgr->print_entries(bs, fp);
	}
}

void print_all_definee(FILE *fp, reaching_def_problem *reach,
		const super_block_cfg *scfg) {
	fprintf(fp,
			"\n_____________all definee<->instr_________________________\n");

	def_catalog *catalog = reach->catalog();
	int num = catalog->num_defs();
	for (int i = 0; i < num; ++i) {
		machine_instr *mi = (machine_instr*) catalog->lookup(i);
		int block_num =
				scfg->in_which_super_block(mi->parent()->list_e())->block_num();
		fprintf(fp, "%d [B_%d] <——> ", i, block_num);
		mi->print(stdout);
		fflush(fp);
	}
	fprintf(fp, "_________________________________________________________\n");
}

/*
 *enroll_variables() --- enroll variable to the bit manager.
 */
void enroll_variables(tree_block *tb, operand_bit_manager *the_bit_mgr) {
	assert(tb->is_block());
	tree_node_list_iter tnli(tb->body());
	while (!tnli.is_empty()) {
		tree_node *tn = tnli.step();
		if (tn->is_instr()) {
			tree_instr *ti = (tree_instr *) tn;
			the_bit_mgr->enroll(ti->instr());
		}
	}
}

/*
 *branch_taken_probability() --- compute branch taken probability计算分支跳转的概率
 */
double branch_taken_probability(cfg_node *cn) {
	if (!cn->is_block()) //如果cn不是基本块，则没有跳转概率
		return 0.0;

	double taken_prob = 0.0;
	cfg_block *cb = (cfg_block *) cn;
	if (cb->ends_in_cbr()) {
		machine_instr *mi = (machine_instr *) cb->in_cti()->instr();
#define KM_DEBUG
#ifdef KM_DEBUG
		fprintf(stderr, "\nopcodestring: %s\n", mi->op_string());
#endif
		immed_list *iml = (immed_list *) (mi->peek_annote(k_instr_profile)); //控制跳转指令profile注释
		if (iml) { //
			int exec_sum = (*iml)[0].integer();
			int taken_sum = (*iml)[1].integer();
			taken_prob =
					(exec_sum != 0) ? ((double) taken_sum / exec_sum) : 0.00001;
		} else
			taken_prob = 0.00001;
	}

	return taken_prob;
}


int get_branch_node_execute_times(cfg_node *cn){
	if (!cn->is_block()) //如果cn不是基本块，则没有跳转概率
			return 0.0;

	int iTimes = 0;
	cfg_block *cb = (cfg_block *) cn;
	assert(cb->ends_in_cbr());
	if (cb->ends_in_cbr()) {
		machine_instr *mi = (machine_instr *) cb->in_cti()->instr();
		immed_list *iml = (immed_list *) (mi->peek_annote(k_instr_profile)); //控制跳转指令profile注释
		if (iml) { //
			int exec_sum = (*iml)[0].integer();
			iTimes = exec_sum;
		}
	}

	return iTimes;
}

/*
 *search_latest_define_point() --- find the latest define point in current block before the position(tree_instr).
 *                               If not find in current block, then find the latest define point from subgraph.
 *在这个cur_pos之前，为operand var_op 寻找最近定值点，
 *现在cur_block中寻找，如果没找到，再从超级块中寻找
 */
tree_instr *search_latest_define_point(operand var_op, tree_instr *cur_pos,
		cfg_block *cur_block, super_block *subgraph) {
	assert(cur_block->contains(cur_pos->list_e()));

	tree_instr *target = NULL;
	int definee = 0;
	if (the_bit_mgr->lookup(var_op, &definee)) {
		cfg_node_instr_iter cnii(cur_block);
		//循环是在当前块中找到最近定义点指令
		while (!cnii.is_empty()) {
			tree_instr *test = cnii.step();
			if (test == cur_pos) {
				break;
			}
			//操作数在目标指令的位置，表示本条指令对给该操作数定值
			else if (test->instr()->dst_op() == var_op) {
				target = test; //不要break，需要多次遍历，找最后的一个
			}
		}

		if (NULL == target) {
			bit_set *all_dp = the_reach->def_points_for(definee); //所有的定义点
			/*printf("va_op  definee and 所对应的定义点:");
			 printf("%d\n", definee);
			 all_dp->print(stdout);
			 printf("\n");*/

			bit_set *all_reach_in = the_reach->reaching_in_set(cur_block); //所有的到达进入集合
			/*printf("reaching_in_set of cur_block:");
			 all_reach_in->print(stdout);
			 printf("\n");*/
			bit_set the_dp;
			the_dp = *all_dp; //将所有的定义点赋值给the_dp
			the_dp *= *all_reach_in; //然后与到达进入集合进行与运算
			/*	printf("the_dp:");
			 the_dp.print(stdout);
			 printf("\n");*/
			bit_set_iter bs_iter(&the_dp);
			while (!bs_iter.is_empty()) {
				int n = bs_iter.step();
				tree_instr *test = the_reach->catalog()->lookup(n)->parent();
				/*printf("包含va_op定义的指令：");
				 ((machine_instr *) test->instr())->print(stdout);
				 printf("\n");*/
				cfg_node_list_iter cnli(subgraph->nodes());
				while (!cnli.is_empty()) {
					cfg_node *cn = cnli.step();
					if (cn->is_block()) {
						cfg_block *cb = (cfg_block *) cn;
						if (cb->contains(test->list_e()))
							target = test;
					}
				}
			}
		}
	}
	/*if (target) {
	 printf("在最可能路径上包含va_op定义的指令：");
	 ((machine_instr *) target->instr())->print(stdout);
	 printf("\n");
	 }*/
	return target;
}

/*
 *search_latest_define_point() --- find the definee's the latest define point from the likely path(subgraph).
 在子图subgraph上寻找最近定义点
 */
bit_set *search_latest_define_point(bit_set *definees, super_block *subgraph,
		bit_set *res) {
	//subgraph->print_relationship(stdout);
	/*printf("\n");
	 printf("definees:");
	 definees->print(stdout);
	 printf("\n");*/
	bit_set_iter def_iter(definees);
	while (!def_iter.is_empty()) {
		int definee = def_iter.step();
//		printf("%d\n", definee);
		bit_set the_dp;
		the_dp = *(the_reach->def_points_for(definee));
		/*printf("definee's def_points:");
		 the_dp.print(stdout);
		 printf("\n");*/
		bit_set_iter the_dp_iter(&the_dp);
		int pos = -1;
		while (!the_dp_iter.is_empty()) {
			int n = the_dp_iter.step();
			tnle *test = the_reach->catalog()->lookup(n)->parent()->list_e();
			/*printf("定义点%d 所在的指令：", n);
			 ((machine_instr *) the_reach->catalog()->lookup(n))->print(stdout);
			 printf("\n");*/
			cfg_node_list_iter cnli(subgraph->nodes());
			while (!cnli.is_empty()) {
				cfg_node *cn = cnli.step();
				if (cn->is_block()) {
					cfg_block *cb = (cfg_block *) cn;
					if (cb->contains(test))
						pos = n;
					/*	printf("pos:");
					 printf("%d", pos);
					 printf("\n");*/

				}
			}
		}
//		printf("针对%d 的定义点\"%d\";\n", definee, pos);

		if (pos != -1)
			res->add(pos);
	}
	/*printf("在最可能路径上找的最近定义点：");
	 res->print(stdout);
	 printf("\n");*/
	return res;
}

bit_set *search_latest_define_point_before_instr(bit_set *definees,
		super_block *subgraph, machine_instr *mi, bit_set *res) {
	bit_set_iter def_iter(definees);
	while (!def_iter.is_empty()) {
		int definee = def_iter.step();

		bit_set the_dp;
		the_dp = *(the_reach->def_points_for(definee));

		bit_set_iter the_dp_iter(&the_dp);
		int pos = -1;
		while (!the_dp_iter.is_empty()) {
			int n = the_dp_iter.step();
			tnle *test = the_reach->catalog()->lookup(n)->parent()->list_e();

			bool done = false;
			cfg_node_list_iter cnli(subgraph->nodes());
			while (!cnli.is_empty() && !done) {
				cfg_node *cn = cnli.step();
				cfg_node_instr_iter instrs(cn);
				while(!instrs.is_empty()){
					tree_instr *ti = instrs.step();
					if((machine_instr *)ti->instr() == mi){
						done = true;
						break;
					}
					if(ti->list_e() == test){
						pos = n;
					}
				}
			}
		}

		if (pos != -1)
			res->add(pos);
	}

	return res;
}

/*
 *search_latest_def_point_before_instr() --- find the latest define point before the tree_instr(pos) in the current block of
 *                                           super block(subgraph).
 在超级基本块的当前基本块之前找最近定义点
 */
tree_instr *search_latest_def_point_before_instr(operand var_op,
		tree_instr *cur_pos, cfg_block *curr_block, super_block *subgraph) {
	super_block *precede_cfg_nodes = new super_block(super_block::BLOCK_LIST);
	cfg_node_list_iter cnli(subgraph->nodes());
	while (!cnli.is_empty()) {
		cfg_node *cn = cnli.step();
		if (cn == curr_block) {
			break;
		} else if (cn->is_block()) {
			precede_cfg_nodes->add_block(cn);
		}
	}

	tree_instr *t_instr = search_latest_define_point(var_op, cur_pos,
			curr_block, precede_cfg_nodes);
	delete precede_cfg_nodes;
	return t_instr;
}

/*
 *find_block_contains_instr() --- find the super block contains the instruction ins.
 找包含该指令的超级块
 */
cfg_block *find_block_contains_instr(super_block *subgraph, machine_instr *mi) {
	cfg_node_list_iter cnli(subgraph->nodes());
	while (!cnli.is_empty()) {
		cfg_node *cn = cnli.step();
		if (cn->is_block()) {
			cfg_block *cb = (cfg_block *) cn;
			if (cb->contains(mi->parent()->list_e()))
				return cb;
		}
	}

	return 0;
}

/*
 *search_related_point() --- find the definee's related live-ins
 */
bit_set *search_related_point(bit_set *live_in, super_block *subgraph,
		bit_set *related) {
	bit_set *res = new bit_set(0, the_reach->catalog()->num_defs());
	bit_set *prev_res = new bit_set(0, the_reach->catalog()->num_defs());

	search_latest_define_point(live_in, subgraph, res);
	/*	printf("definees 最近定义点：");
	 res->print(stdout);
	 printf("\n");*/
	while (!res->is_empty()) {
		*related += *res;
		*prev_res = *res;
		res->clear();
		bit_set_iter iter(prev_res);
		while (!iter.is_empty()) {
			int n = iter.step();
			machine_instr *m_instr =
					(machine_instr *) the_reach->catalog()->lookup(n);
			cfg_block *cb = find_block_contains_instr(subgraph, m_instr);
			//printf("最近定义点在块%d\n：", cb->number());

			/**
			 * 处理src_op，
			 * 对于乘法和除法指令，例如：
			 * mult src1, src2
			 * mflo dest
			 * 需要单独处理
			 * 由于mult src1, src2没有被def_catalog登记，我们只有从m_instr本身计算其上一条指令
			 * 由于def_catalog没有登记mult指令，所以res不会有mult的登记点，所以之后的统计依赖数时
			 * 也只统计mflo，不能统计mflo。
			 * 这本身不是一个大不了的问题，只需紧记我们将上述两条指令当一条指令处理即可，即将两者绑定
			 * 后面pslice的构造也需要和此处处理保持一致
			 */
			switch (m_instr->opcode()) {
			case mo_mflo:
			case mo_mfhi:
			{
				machine_instr *mi = get_immediate_pre_instr_in_block(m_instr);
				if ((int) mi->opcode() == mo_mult
						|| (int) mi->opcode() == mo_multu) {
					m_instr = mi;
				}
				break;
			}
			}

			seach_related_point_for_opd(m_instr, cb, subgraph, res);
		}
	}
	return related;
}



/*
 *search_related_point_before_instr() --- find the definee's related live-ins
 */
bit_set *search_related_point_before_instr(bit_set *live_in,
		super_block *subgraph, machine_instr *cqip_instr, bit_set *related) {
	bit_set *res = new bit_set(0, the_reach->catalog()->num_defs());
	bit_set *prev_res = new bit_set(0, the_reach->catalog()->num_defs());

	search_latest_define_point_before_instr(live_in, subgraph, cqip_instr, res);
	printf("definees 最近定义点：");
	res->print(stdout);
	printf("\n");
	while (!res->is_empty()) {
		*related += *res;
		*prev_res = *res;
		res->clear();
		bit_set_iter iter(prev_res);
		while (!iter.is_empty()) {
			int n = iter.step();
			machine_instr *m_instr =
					(machine_instr *) (the_reach->catalog()->lookup(n));
			printf("m_instr:");
			m_instr->print(stdout);
			printf("\n");
			cfg_block *cb = find_block_contains_instr(subgraph, m_instr);
			//printf("最近定义点在块%d\n：", cb->number());
			/**
			 * 处理src_op，
			 * 对于乘法和除法指令，例如：
			 * mult src1, src2
			 * mflo dest
			 * 需要单独处理
			 * 由于mult src1, src2没有被def_catalog登记，我们只有从m_instr本身计算其上一条指令
			 * 由于def_catalog没有登记mult指令，所以res不会有mult的登记点，所以之后的统计依赖数时
			 * 也只统计mflo，不能统计mflo。
			 * 这本身不是一个大不了的问题，只需紧记我们将上述两条指令当一条指令处理即可，即将两者绑定
			 * 后面pslice的构造也需要和此处处理保持一致
			 */
			switch (m_instr->opcode()) {
			case mo_mflo:
			case mo_mfhi: {
				machine_instr *mi = get_immediate_pre_instr_in_block(m_instr);
				if ((int) (mi->opcode()) == mo_mult
						|| (int) (mi->opcode()) == mo_multu) {
					m_instr = mi;
				}
				break;
			}
			}

			seach_related_point_for_opd(m_instr, cb, subgraph, res);

		}
	}
	return related;
}


/**
 * 为m_instr的操作数寻找定义点
 */
void seach_related_point_for_opd(machine_instr *m_instr, cfg_block *cb,
		super_block *subgraph, bit_set *& res) {
	for (int i = 0; i < m_instr->num_srcs(); i++) {
		operand opd = m_instr->src_op(i);

		//如果opd本身有时一条地址指令,我们应该取出其指令的所有opd
		if (Is_ea_operand(opd)) {
			seach_related_point_for_opd((machine_instr*)opd.instr(), cb, subgraph, res);

		}
		if (opd.is_symbol() || opd.is_reg()) {

			tree_instr *t_ins = search_latest_def_point_before_instr(opd,
					m_instr->parent(), cb, subgraph);
			if (t_ins != 0) {
				/*printf("最近定义opd的指令：");
				 ((machine_instr *) t_ins->instr())->print(stdout);
				 printf("\n");*/
				int pos = the_reach->catalog()->first_id(
						(machine_instr *) t_ins->instr());
				/*printf("依赖指令第一个定义点的id：");
				 printf("%d\n", pos);*/

				res->add(pos);
				/*	printf("res:");
				 res->print(stdout);
				 printf("\n");*/
			}
		}
	}
}




/**
 * get immdediate predecussor tnle* by current tnle
 * pos不是块内第一条指令
 */
machine_instr *get_immediate_pre_instr_in_block(machine_instr *other_mi) {
	assert(NULL != other_mi);
	tnle * t_e = other_mi->parent()->list_e();
	assert(NULL != t_e);
	cfg_node *node = get_cfg_node_by_tnle(t_e);
	assert(node->is_block());

	cfg_block *cb = (cfg_block*) node;
	assert(cb->contains(t_e));
	//此处保证一定存在

	cfg_node_instr_iter instrs(node);
	machine_instr *target = NULL;
	while (!instrs.is_empty()) {
		tree_instr *ti = instrs.step();
		machine_instr *mi = (machine_instr*) ti->instr();
		if (other_mi == mi) {
			break;
		}
		target = mi;
	}

	assert(NULL != target);
	//如果target==NULL，说明pos是第一个tnle*

	return target;
}

