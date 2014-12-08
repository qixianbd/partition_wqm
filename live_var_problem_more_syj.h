/*
 * live_var_problem_more_syj.h
 * 说明：
 * 多加这一层继承，就是希望使用live_var_problem中的protected成员 bit_set_array *defs;
 * 同时希望使用live_var_problem_more的print函数，
 * 但是我不能修改machsuif库，否则其他机器上不兼容
 *
 * 经过多方测试，live_var_problem_more中的all_def不能表示一个基本块内部的def，应该是表示其他意义
 * 暂时没有发现其他用途。
 * 所以：
 * 1，将all_def的使用屏蔽掉
 * 2.将def暴露出来
 *  Created on: 2012-3-8
 *      Author: harry
 */

#ifndef LIVE_VAR_PROBLEMMORE_SYJ_H_
#define LIVE_VAR_PROBLEMMORE_SYJ_H_

#include <suif_copyright.h>
#include "common.h"

class live_var_problem_more_syj: public live_var_problem_more {
public:
	live_var_problem_more_syj(cfg *graph, operand_bit_manager *opd_bit_man,
			instruction_def_use_f du_fun) :
			live_var_problem_more(graph, opd_bit_man, du_fun) {

	}

	/*
	 * 屏蔽掉live_var_problem_more
	 */
	bit_set *all_def_set(cfg_node *cn) {
		assert(0);
		return NULL;
	}

	/**
	 * 增加此类的目的就是希望将defs暴露出来使用
	 * 而不使用all_def
	 */
	bit_set *def_set(cfg_node *cn) {
		return (*defs)[cn->number()];
	}

};

#endif /* LIVE_VAR_PROBLEMMORE_SYJ_H_ */
