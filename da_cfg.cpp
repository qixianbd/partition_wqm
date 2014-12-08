#include <suif_copyright.h>
#include <stdio.h>
#include <suif1.h>
#include <machine.h>
#include <string.h>
#include <algorithm>

#include "da_cfg.h"
#include "spmt_utility.h"
#include "misc.h"
#include "min_cut_fun.h"

/*
 *da_cfg::da_cfg() --- constructor of cfg contains data analysis 
 */
da_cfg::da_cfg(tree_block *b, boolean build_blocks, boolean break_at_call,
		boolean keep_layout) :
		cfg(b, build_blocks, break_at_call, keep_layout) {
	liveness_more = NULL;
}

/*
 *da_cfg::~da_cfg() --- destructor of cfg contains data analysis.
 */
da_cfg::~da_cfg() {
	delete liveness_more;
}

/*
 *da_cfg::find_live_vars_more() --- find live vars according to the operand bit manager
 */
void da_cfg::find_live_vars_more(operand_bit_manager *bit_man) {
	delete liveness_more;
	liveness_more = new live_var_problem_more_syj(this, bit_man,
			Instruction_def_use);
}

/*
 *da_cfg::print_cfg() --- print da_cfg
 */
void da_cfg::print_cfg(FILE *fp) {
	for (unsigned i = 0; i < num_nodes(); i++) {
		cfg_node *cn = node(i);
		if (cn->is_begin())
			((cfg_begin *) cn)->print();
		if (cn->is_block()) {
			((cfg_block *) cn)->print_with_instrs();
			printf("immed_postdom: Block %d\n\n", immed_postdom(cn)->number());
		}
		if (cn->is_end())
			((cfg_end *) cn)->print();
	}
}

/*
 *da_cfg::print_dfa() --- print data flow analysis.
 */
void da_cfg::print_dfa(FILE *fp) {
	liveness_more->dump(fp);
}

/**
 *da_cfg::find_liveins() --- find the super block sb's live-ins.
 *2012-03-08 将所有对def_all_set()调用改成了def_set(cn)
 */
void da_cfg::find_liveins(super_block *sb, operand_bit_manager *bit_man,
		bit_set *livein) {
	bit_set def_g(0, bit_man->num_bits()); //全局所有定义的
	bit_set use_b(0, bit_man->num_bits()); //当前块内使用的
	bit_set def_b(0, bit_man->num_bits()); //当前块内定义的
	bit_set use_undef(0, bit_man->num_bits()); //当前块使用了但是没有定义
	def_g.clear();
	use_undef.clear();
	cfg_node_list_iter cnli(sb->nodes());
	while (!cnli.is_empty()) {
		cfg_node *cn = cnli.step();

		def_g += *(def_set(cn));
		def_b = *(def_set(cn));
		use_b = *(use_set(cn));

		bit_set_iter use_iter(&use_b);

		while (!use_iter.is_empty()) {
			int n = use_iter.step();
			if (!def_g.contains(n)) {
				use_undef.add(n);
			}
		}
	}

	livein->copy(&use_undef);
}


/*
 *da_cfg::find_liveins() --- find the super block sb's live-ins.
 *2012-03-08 将所有对def_all_set()调用改成了def_set(cn)
 */
void da_cfg::find_liveins_and_defs(super_block *sb, operand_bit_manager *bit_man,
		bit_set *livein, bit_set *def) {
	bit_set def_g(0, bit_man->num_bits()); //全局所有定义的
	bit_set use_b(0, bit_man->num_bits()); //当前块内使用的
	bit_set def_b(0, bit_man->num_bits()); //当前块内定义的
	bit_set use_undef(0, bit_man->num_bits()); //使用了但是没有定义
	def_g.clear();
	use_undef.clear();
	cfg_node_list_iter cnli(sb->nodes());
	while (!cnli.is_empty()) {
		cfg_node *cn = cnli.step();

		def_g += *(def_set(cn));
		def_b = *(def_set(cn));
		use_b = *(use_set(cn));

		bit_set_iter use_iter(&use_b);

		while (!use_iter.is_empty()) {
			int n = use_iter.step();
			if (!def_g.contains(n)) {
				use_undef.add(n);
			}
		}
	}

	livein->copy(&use_undef);
	def->copy(&def_g);
}


/*
 * 为cn中从mi开始（包括mi）的后部分代码段计算live_in和def
 */
void da_cfg::find_liveins_after_pos(super_block *cqip_path, machine_instr *mi, operand_bit_manager *bit_man,
   		bit_set *livein){
	assert(NULL != cqip_path);
	assert(NULL != mi);
	assert(NULL != livein);

	//计算第一部分的live_ins
	bit_set first_livein(0, bit_man->num_bits()); //使用了但是没有定义
	bit_set first_def(0, bit_man->num_bits()); //全局所有定义的
	first_livein.clear();
	first_def.clear();
	cfg_node *cn = get_cfg_node_by_tnle(mi->parent()->list_e());
	this->find_cn_liveins_and_defs_after_pos(cn, mi, bit_man, &first_livein, &first_def);

	//计算第二部分live_ins
	bit_set second_livein(0, bit_man->num_bits()); //使用了但是没有定义
	bit_set second_def(0, bit_man->num_bits()); //全局所有定义的
	second_livein.clear();
	second_def.clear();
	cfg_node_list *back_cnl = sub_cnl(cqip_path->nodes(), cn);
	back_cnl->pop();//去掉第一个
	super_block *back_sb = new super_block();
	back_sb->add_blocks(back_cnl);
	this->find_liveins_and_defs(back_sb, bit_man, &second_livein, &second_def);

	//集合运算最终结果存放在first_livein和first_def中
	second_livein -= first_def;
	first_livein += second_livein;

	livein->copy(&first_livein);
	delete back_sb;
}


/*
 * 为cn中从mi开始（包括mi）的后部分代码段计算live_in和def
 */
void da_cfg::find_liveins_and_def_after_pos(super_block *cqip_path, machine_instr *mi, operand_bit_manager *bit_man,
   		bit_set *livein, bit_set *def){
	assert(NULL != cqip_path);
	assert(NULL != mi);
	assert(NULL != livein);

	//计算第一部分的live_ins
	bit_set first_livein(0, bit_man->num_bits()); //使用了但是没有定义
	bit_set first_def(0, bit_man->num_bits()); //全局所有定义的
	first_livein.clear();
	first_def.clear();
	cfg_node *cn = get_cfg_node_by_tnle(mi->parent()->list_e());
	this->find_cn_liveins_and_defs_after_pos(cn, mi, bit_man, &first_livein, &first_def);

	//计算第二部分live_ins
	bit_set second_livein(0, bit_man->num_bits()); //使用了但是没有定义
	bit_set second_def(0, bit_man->num_bits()); //全局所有定义的
	second_livein.clear();
	second_def.clear();
	cfg_node_list *back_cnl = sub_cnl(cqip_path->nodes(), cn);
	back_cnl->pop();//去掉第一个
	super_block *back_sb = new super_block();
	back_sb->add_blocks(back_cnl);
	this->find_liveins_and_defs(back_sb, bit_man, &second_livein, &second_def);

	//集合运算最终结果存放在first_livein和first_def中
	second_livein -= first_def;
	first_livein += second_livein;

	first_def += second_def;

	livein->copy(&first_livein);
	def->copy(&first_def);
	delete back_sb;
}


/*
 * 为cn中从mi开始（包括mi）的后部分代码段计算live_in和def
 */
void da_cfg::find_cn_liveins_and_defs_after_pos(cfg_node *cn, machine_instr *mi, operand_bit_manager *bit_man,
		bit_set *livein, bit_set *def) {
	assert(NULL != cn);
	assert(NULL != mi);
	bool is_begin = false;

	bit_set def_g(0, bit_man->num_bits()); //全局所有定义的
	bit_set use_b(0, bit_man->num_bits()); //当前块内使用的
	bit_set def_b(0, bit_man->num_bits()); //当前块内定义的
	bit_set use_undef(0, bit_man->num_bits()); //使用了但是没有定义
	def_g.clear();
	use_undef.clear();

	cfg_block *block = (cfg_block *) cn;
	cfg_node_instr_iter cnii(block);
	while (!cnii.is_empty()) {
		machine_instr *mi_in_block = (machine_instr *) (cnii.step())->instr();
		if (mi == mi_in_block){
			is_begin = true;
		}

		if(is_begin){
			def_b.clear();
			use_b.clear();
			Instruction_def_use(mi_in_block, bit_man, &def_b, &use_b);//此函数在live_var.h定义
			printf("mi_in_block :\n");
			mi_in_block->print(stdout);
			printf("\n");
			printf("def_b:\n");
			def_b.print(stdout);
			printf("\n");
			printf("use_b:\n");
			use_b.print(stdout);
			printf("\n");

			bit_set_iter use_iter(&use_b);
			while (!use_iter.is_empty()) {
				int n = use_iter.step();
				if (!def_g.contains(n)) {
					use_undef.add(n);
				}
			}
			//一定要放在use_undef之后，否则a1 = a1 + a2中a1无法识别
			def_g += def_b;
		}

	}
	livein->copy(&use_undef);
	def->copy(&def_g);
}


//////////////////////////////////////////////////////////////////////////


/**
 * mi将循环一分为2， 我们不妨看作两部分A->B,现在要计算(B->A)的live_in
 * livein(B->A) = livein(B) + (livein(A)-def(B)) = livein(B) + livein(B) + (livein(A)-def(B))
 *        	= livein(B) + livein(B) + (livein(A)-def(B))
 *        	= livein(B) + (live_in(A) + live(B) -def(B))
 *        	=livein(B)+live(A->B)-def(B)
 *所以，第一步计算处live_in(B)和def(B)
 */
void da_cfg::find_loop_liveins_at_cqip_pos(super_block *loop_path, machine_instr *mi, operand_bit_manager *bit_man,
   		bit_set *livein){
	assert(NULL != loop_path);
	assert(NULL != mi);
	assert(NULL != livein);

	//计算cqip_pos开始的后半部分的live_in和use
	bit_set first_livein(0, bit_man->num_bits()); //使用了但是没有定义
	bit_set first_def(0, bit_man->num_bits()); //全局所有定义的
	first_livein.clear();
	first_def.clear();
	this->find_liveins_and_def_after_pos(loop_path, mi, bit_man, &first_livein,
			&first_def);
	printf("从mi开始的后部分live_in:\n");
	first_livein.print(stdout);
	printf("\n");
	printf("从mi开始的后部分def:\n");
	first_def.print(stdout);
	printf("\n");

	//计算整块的所有节点的的live_ins(B)和def(B)
	bit_set second_livein(0, bit_man->num_bits()); //使用了但是没有定义
	second_livein.clear();
	this->find_liveins(loop_path, bit_man, &second_livein);

	printf("整个部分live_in:\n");
	second_livein.print(stdout);
	printf("\n");

	//集合运算,此两步结果写入
	second_livein -= first_def;
	first_livein += second_livein;

	printf("最终live_in:\n");
	first_livein.print(stdout);
	printf("\n");
	livein->copy(&first_livein);
}

/*
 *da_cfg::find_liveins_of_loop() -- find the loop block's live-ins.
 */

/*void da_cfg::find_liveins_of_loop(loop_block *loop,
		operand_bit_manager *bit_man, bit_set *livein) {
	bit_set use(0, bit_man->num_bits());
	bit_set def(0, bit_man->num_bits());

	cfg_block *cn = (cfg_block *) loop->end_node();
	//use = *(live_out_set(cn));
	//use = *(use_set(cn));
	//def = *(all_def_set(cn));
	//use += def;

	if (cn->ends_in_cti()) {
		machine_instr *mi_ctl =*/
	//			(machine_instr *) ((cfg_block *) cn)->in_cti()->instr();
		/*  printf("########################################################");
		 mi_ctl->print(stdout);*/
/*		for (int i = 0; mi_ctl && i < mi_ctl->num_srcs(); i++) {
			operand opd = mi_ctl->src_op(i);
			if (opd.is_symbol() || opd.is_reg()) {
				bit_man->insert(opd, &use);
			}*/
			/*
			 if(opd.is_instr())
			 {
			 machine_instr *mi = (machine_instr *)opd.instr();
			 printf("#########################################################");
			 mi->print(stdout);
			 bit_man->insert(mi->dst_op(), &use);
			 for(int i = 0; i < mi->num_srcs(); i++)
			 {
			 operand opd1 = mi->src_op(i);
			 if(opd.is_reg() || opd.is_symbol())
			 bit_man->insert(opd, &use);
			 }
			 }
			 if(opd.is_expr())
			 {

			 }
			 */
/*		}
	}
	if (cn->ends_in_ubr()) {
		find_liveins((super_block *) loop, bit_man, &use);
	}

	livein->copy(&use);
}*/

/*
 *da_cfg::generate_dot() -- generate cfg graph for dot, like as VCG
 */
void da_cfg::generate_dot(FILE *fp /*= stdout*/) {
	static unsigned int Subgraph_ID = 0;
	static unsigned int Block_base = 0;

	char proc_name[32];
	char temp[16], temp2[16];

	fprintf(fp, "subgraph cluster_%s{\n", itoa(Subgraph_ID++, temp, 10));
	fprintf(fp, "\tlabel = \"%s\"", tproc()->proc()->name());
	fprintf(fp, "\tstyle = \"dashed\";\n");
	fprintf(fp, "\tcolor = purple;\n");

	for (int i = 0; i < num_nodes(); i++) {
		cfg_node *cn = node(i);
		if (cn->is_begin()) {
			fprintf(fp, "\tNode%s [label = \"%s\"];\n",
					itoa(Block_base + cn->number(), temp, 10), "Begin");

		} else if (cn->is_end()) {
			fprintf(fp, "\tNode%s [label = \"%s\"];\n",
					itoa(Block_base + cn->number(), temp, 10), "End");
		} else {
			fprintf(fp, "\tNode%s [label = \"B%s_%d\"];\n",
					itoa(Block_base + cn->number(), temp, 10),
					itoa(cn->number(), temp2, 10),
					cfg_node_static_instr_size(cn));
		}
		fprintf(fp, "\tNode%s -> {", itoa(Block_base + cn->number(), temp, 10));

		cfg_node_list_iter cnl_iter(cn->succs());
		while (!cnl_iter.is_empty()) {
			cfg_node *succ_cn = cnl_iter.step();
			//for the begin block, there is edge from begin block to the end block, so we must delete this edge from cfg.
			if (cn->is_begin() && succ_cn->is_end())
				continue;

			fprintf(fp, "Node%s ",
					itoa(Block_base + succ_cn->number(), temp, 10));
		}
		fprintf(fp, "}\n");
	}
	fprintf(fp, " }\n");
	Block_base += num_nodes();
}

void da_cfg::generate_dot(char *procedureName, PrintGraphFlag suffixFlag)
{
	//addByKeyming 20140513
	char filename_before_spmt_instr[256];
	strncpy(filename_before_spmt_instr, "testcase/functionDot/", 256);
	strncat(filename_before_spmt_instr, procedureName, 256);
	char suffixStr[32];
	if(suffixFlag == BEFORE){
		strncpy(suffixStr, "_prev.dot", strlen("_prev.dot")+1);
	}
	else if(suffixFlag == AFTER){
		strncpy(suffixStr, "_after.dot", strlen("_after.dot")+1);
	}
	strncat(filename_before_spmt_instr, suffixStr, strlen(suffixStr));
	FILE *func_dot = fopen(filename_before_spmt_instr, "w");
	if(!func_dot){
		debug(0, "Cannot create the dotFunction file for the procedure %s", procedureName);
	}
	the_cfg->generate_full_instr_dot(func_dot);
	fclose(func_dot);
}
/**
 * 输出一条指令到fp. 对于普通指令in, 只输出 opcode des_operand src_operand. 对于 标签, 输出标签.
 * @param in
 * @param fp
 * 依赖的全局变量: 无
 */
void print_instruction(instruction* in, FILE *fp = stdout)
{
	assert(in);

	machine_instr *mi = dynamic_cast<machine_instr *> (in);
	if(mi != NULL){
		mi_rr *mirr = dynamic_cast<mi_rr *>(mi);
		if(mirr != NULL){// if mi is mi_rr

			mi_bj *mibj = dynamic_cast<mi_bj *>(mirr);
			if(mibj != NULL){
				//if mi is a jump instruction.

				//1. print the opcode.
				fprintf(fp, "%s\t", in->op_string());

				//2. print the target lable.
				if(mibj->target() != NULL){
					Print_symbol(fp, mibj->target());

				}
				//mibj->print();
				fprintf(fp, "\\n");
				fflush(fp);
				return ;
			}// end of if mi is bj instruction.

			//1. print the opcode.
			fprintf(fp, "%s\t", in->op_string());

			//2. print the dest operand.
			if(mi->num_dsts() ){
				operand opd = mi->dst_op(0);
				if(!opd.is_null()){
						opd.print(in, fp);
						fprintf(fp, "\t");
				}
			}

			//3. print the all (could be more than one) source operand.
			int operand_num = mi->num_srcs();
			for(int i = 0; i < operand_num; i++){
				operand opd = mi->src_op(i);
				if(!opd.is_null()){
					opd.print(in, fp);
					fprintf(fp, "\t");
				}
			}
			fprintf(fp, "\\n");

		} // end of if mirr != NULL
		else{

			mi_lab *milab = dynamic_cast<mi_lab *>(mi);
			if(milab){
				//milab->print(fp);
				milab->label()->sym_node::print(fp);
				fprintf(fp,":\\n");			/* mips-specific ending */
			   // mips_print_reloc(milab, fp);
			}
		}
	}
	fflush(fp);

}
/**
 * 生成一个打印有全部指令的cfg dot 文件. 该函数会打印处每个基本块的所有指令, 标签.  每一个基本块被当作一个node 处理
 * @param fp 输出结果的文件指针. Point to the output file.
 */
void da_cfg::generate_full_instr_dot(FILE* fp)
{
	char proc_name[32];
	char temp[16], temp2[16];

	std::vector<int> spawnNodes;
	std::vector<int> cqipNodes;


	fprintf(fp, "digraph G{\n");
	fprintf(fp, "\tlabel = \"%s\"", tproc()->proc()->name());
	fprintf(fp, "\tstyle = \"dashed\";\n");
	fprintf(fp, "\tcolor = purple;\n");

	for(int i = 0; i < this->num_nodes(); i++){
		cfg_node *cn = node(i);
		if (cn->is_begin()) {
			fprintf(fp, "\tNode%s [label = \"%s\"];\n",
					itoa(cn->number(), temp, 10), "Begin");
			//((cfg_begin *) cn)->print();

		} else if (cn->is_end()) {
			fprintf(fp, "\tNode%s [label = \"%s\"];\n",
					itoa(cn->number(), temp, 10), "End");
			//((cfg_end *) cn)->print(fp);
		} else {
			fprintf(fp, "\tNode%s [label = \"B%s_%d\\n",
					itoa(cn->number(), temp, 10),
					itoa(cn->number(), temp2, 10),
					cfg_node_static_instr_size(cn));
	//		((cfg_block *) cn)->print_with_instrs(stdout);
			//puts("");

		    cfg_node_instr_iter cnii(cn);
		    while (!cnii.is_empty()) {
				instruction *in = cnii.step()->instr();
				//in->instruction::print(fp, 1, 1);

				print_instruction(in, fp);
				/*
				fprintf(fp, "%s\t", in->op_string());
				//print the operand.
				machine_instr *mi = dynamic_cast<machine_instr *> (in);
				if(mi != NULL){
					if(mi->num_dsts() ){
						operand opd = mi->dst_op(0);
						if(!opd.is_null()){
							opd.print(in, fp);
							fprintf(fp, "\t");
						}
					}
					int operand_num = mi->num_srcs();
					for(int i = 0; i < operand_num; i++){
						operand opd = mi->src_op(i);
						if(!opd.is_null()){
							opd.print(in, fp);
							fprintf(fp, "\t");
						}
					}

					//fprintf(fp, "%d\t", mi->num_dsts());
					//fprintf(fp, "\\n");
				}
				else{
					in_lab *inlab = dynamic_cast<in_lab *>(in);
					if(inlab){
						inlab->print(fp);
					}
				}
				fprintf(fp, "\\n");
				fflush(fp);

				*/


				/**
				 * 测试该基本块是否包含spawn, cqip等特殊指令. 如果包含, 则加入到特殊的vector里.
				 */
				if(strcmp(in->op_string(), "spawn") == 0){
					spawnNodes.push_back(cn->number());
				}
				else if(strcmp(in->op_string(), "cqip") == 0){
					cqipNodes.push_back(cn->number());
				}
		    }
		    fprintf(fp, "\"];\n");
		}
/**
 * 下面打印的是节点之间的指向关系.
 */

		fprintf(fp, "\tNode%s -> {", itoa(cn->number(), temp, 10));

		cfg_node_list_iter cnl_iter(cn->succs());
		while (!cnl_iter.is_empty()) {
			cfg_node *succ_cn = cnl_iter.step();
			//for the begin block, there is edge from begin block to the end block, so we must delete this edge from cfg.
			if (cn->is_begin() && succ_cn->is_end())
				continue;

			fprintf(fp, "Node%s ",
					itoa(succ_cn->number(), temp, 10));
		}
		fprintf(fp, "}\n");

	}
	// 此时已经退出了for loop.
	/**
	 * 下面对于包含spawn 和 cqip的点进行特殊颜色 特殊方框的打印处理.
	 */

	//对于一个基本块, 如果只包含spawn 打red, 只包含cqip打 yellow, contain the both , print blue.
	for(int i = 0; i < spawnNodes.size(); i++){
		if(std::find(cqipNodes.begin(), cqipNodes.end(), spawnNodes[i]) == cqipNodes.end()){
			fprintf(fp, "\tNode%s [shape = box ,style=filled ,color=red];\n",
					itoa(spawnNodes[i], temp, 10));
		}
		else{
			fprintf(fp, "\tNode%s [shape = ellipse ,style=filled ,color=blue];\n",
					itoa(spawnNodes[i], temp, 10));
		}
	}

	for(int i = 0; i < cqipNodes.size(); i++){
		if(std::find(spawnNodes.begin(), spawnNodes.end(), cqipNodes[i]) == spawnNodes.end()){
			fprintf(fp, "\tNode%s [shape = polygon ,style=filled ,color=yellow];\n",
					itoa(cqipNodes[i], temp, 10));
		}

	}


	fprintf(fp, "}\n");
}

