#include <iostream>
#include <fstream>
#include <stack>
#include <list>
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>

//#include <string>
#include <map>

#include "common.h"
#include "da_cfg.h"
#include "super_block.h"
#include "super_block_cfg.h"
#include "loop_block.h"
#include "misc.h"
#include "spmt_instr.h"
#include "spmt_fun.h"
#include "spawn_pos_trace.h"
#include "threshold.h"
#include "thread.h"
#include "super_block_path.h"
#include "XmlDocument.h"
#include "XmlElement.h"
#include "node.h"
#include "helper.h"
#include "min_cut.h"
#include "min_cut_fun.h"
#include "loop_cut.h"

using namespace std;

extern char *prog_ver_string;

/*
 *global variables
 */
FILE *dot_fp; /*dot file of cfg's pointer */
FILE *stat_fp; /*statistics file's pointer */
FILE *test_fp; /*syj*/
FILE *loop_fp; /*获取loop的中间数据*/

/** \brief: statistics information for *.xml file 
 */
std::ofstream outfile;
XmlDocument *xmlfile;
XmlElement *root, *curr_thread_element, *loop_element, *pslice_element,
		*subnode;
int sum_of_thread_size = 0, sum_of_loop_size = 0, num_of_threads = 0,
		num_of_loops = 0;
int pslice_of_thread = 0, pslice_of_loop = 0;

mproc_symtab *cur_psymtab = NULL; /*current wrapped procedure symbol table */

da_cfg *the_cfg = NULL; /*current procedure da_cfg */
operand_bit_manager *the_bit_mgr = NULL; /*operand bit manager */
cfg_node_list *the_full_cnl = NULL; /*the cfg node list of full cfg */
reg_def_teller *the_teller = NULL; /*regiter define teller */
reaching_def_problem *the_reach = NULL; /*reaching define problem */
super_block_cfg *the_scfg = NULL; /*super block cfg composed of super block */


std::vector<loop_block *>*loops = 0; /*vector which contains loop super block */
std::vector<thread *>*threads = 0; /*vector which contains thread */

/*
 *Debugging help
 */
int debuglvl = 0; /*level of diagnostics to print out */
char *progname; /*name of the program */

/* ------------ Input/output stream routines -------------- */

/* Convert_to_mi() -- helper routine for Read_machine_proc(). */
/*static machine_instr *Convert_to_mi(instruction * i) {
 unsigned j;
 int m_opcode;
 machine_instr *mi = NULL;
 immed_list *il = (immed_list *) i->get_annote(k_machine_instr);
 assert(il);

 switch (i->opcode()) {
 case io_gen:  generic
 assert(((in_gen *) i)->name() != k_suif);
 m_opcode = (*il)[1].integer();  get opcode

 create machine instruction based on in_gen
 switch (which_mformat(m_opcode)) {
 case mif_xx:
 mi = new mi_xx(m_opcode);
 if ((il = (immed_list *) i->get_annote(k_instr_xx_sources))) {
 while (!il->is_empty())
 ((mi_xx *) mi)->append_operand(il->pop());
 delete il;
 }
 break;

 case mif_lab:
 assert_msg(FALSE, ("Convert_to_mi() -- found io_gen label"));
 break;
 case mif_bj:
 mi = new mi_bj(m_opcode, NULL);
 if ((il = (immed_list *) i->get_annote(k_instr_bj_target))) {
 branch/jump has symbolic target, transfer it
 sym_node *t = (*il)[0].symbol();
 ((mi_bj *) mi)->set_target(t);
 delete il;
 }
 break;

 case mif_rr:
 mi = new mi_rr(m_opcode);
 break;

 default:
 assert_msg(FALSE, ("Convert_to_mi() -- unknown instr format"));
 break;
 }

 transfer result_type
 mi->set_result_type(i->result_type());

 transfer generic destination operands
 mi->set_num_dsts(i->num_dsts());
 for (j = 0; j < mi->num_dsts(); j++) {
 assert(!i->dst_op(j).is_instr());
 mi->set_dst(j, i->dst_op(j));
 }

 transfer generic source operands
 mi->set_num_srcs(i->num_srcs());
 for (j = 0; j < i->num_srcs(); j++) {
 if (i->src_op(j).is_instr()) {
 transfer immediate operand or expression tree
 * representing an effective address operand
 instruction *eai = i->src_op(j).instr();
 i->src_op(j).remove();  clear pointers
 i->set_src_op(j, operand());
 mi->set_src_op(j, operand(eai));

 } else
 mi->set_src_op(j, i->src_op(j));
 }

 break;

 case io_lab:  label
 mi = new mi_lab((*il)[1].integer(), (in_lab *) i);
 break;

 default:
 assert_msg(FALSE, ("Convert_to_mi() -- unexpected SUIF opcode"));
 break;
 }

 assert(mi);  sanity check

 transfer any remaining annotations
 i->copy_annotes(mi);

 return mi;
 }*/

/*
 *debug() -- print out a diagnostic message.
 */
void debug(const int lvl, const char *msg...)
{
	if (lvl > debuglvl)
	return;
	va_list ap;
	va_start(ap, msg);
	fprintf(stderr, "%s: DEBUG (%d):", progname, lvl);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	putc('\n', stderr);
	fflush(stderr);
}

/*
 *global variables.
 */
char *k_cqip_pos; /*control quasi-independent point */
char *k_proc_profile; /*procedure dynamic cycles profile annotation */
char *k_instr_profile; /*control jump instruction profile annotation */
char *k_pslice_exit_pos; /*pre-compute slice exit position annotation */
char *k_pslice_entry_pos; /*pre-compute slice entry position annotation */
char *k_call_overhead; /*调用开销caller procedure  dynamic cycles annotation */
char *k_looppos;   /* the begin of a loop*/
char *k_loopend;     /* the end of a loop (one loop may have several loopends)*/

boolean GenerateGraph = FALSE; /*generate dot file of cfg */
boolean verbose = FALSE; /*verbose mode */
boolean statistics = FALSE; /*statistics information */
boolean usexml = FALSE; /*statistics information stored in xml file */

//static machine_instr *Convert_to_mi(instruction * i);
/*
 *definitions of local procedures.
 */
static void init_mpartition();

static super_block_cfg *construct_super_blocks();

static void construct_super_block_relationship(super_block_cfg * sbc);
static cfg_node *peek_node(super_block * sb);

//thread* create_new_thread(super_block *pdom, tnle *spawn_pos, thread *curr_thread, super_block *spawn_path);

static void write_spawn_info();

static char *New_filename(char *ifile, char //construct loop region
		*ext);
static void Process_symtab(base_symtab *, boolean, FILE *);
static void Process_file(file_set_entry *);
static void Process_procedure(proc_sym *cur_psym);

/*
 *main() --- main function.
 */
int main(int argc, char **argv) {
	start_suif(argc, argv);
	//    init_machine(argc, argv);

	/** \brief: read threshold parameter from threshold file
	 */
//	threshold::parse_file();
//	threshold::print();

	/** \brief: initialize annotation
	 */
	init_mpartition();

	progname = *argv;
	argv++;
	argc--;

	if (!argc) {
		fprintf(stderr, "\nUsage is:\n");
		fprintf(stderr, "  %s [options...] infile [outfile]\n", progname);
		fprintf(stderr, "\nOptions are: \n");
		fprintf(stderr, "\t-G\tprint dot file of cfg\n");
		fprintf(stderr, "\t-V\tverbose mode\n");
		fprintf(stderr, "\t-S\tstatistics information\n\n");
		fprintf(stderr,
				"\t-X\tstatistics information stored in *.xml file\n\n");
		fprintf(stderr, "\nVERSION: %s\n", prog_ver_string);
		exit(1);
	}
	while (argc) {
		if (**argv == '-') {
			char *arg = argv[0];
			//debug information
			if (strncmp(arg, "-debug", 6) == 0) {
				if (arg[6])
					debuglvl = atol(&arg[6]);
				else if (argc < 1 || argv[1][0] == '-')
					assert_msg(FALSE,
							("-debug option requires a debugging level"));
				else {
					debuglvl = atol(*++argv);
					argc--;
				}
				debug(1, "Debug level is %d", debuglvl);
			}
			//print dot file of the cfg
			else if (strcmp(arg, "-G") == 0)
				GenerateGraph = TRUE;
			//verbose mode
			else if (strcmp(arg, "-V") == 0)
				verbose = TRUE;
			//statistics information
			else if (strcmp(arg, "-S") == 0)
				statistics = TRUE;
			//statistics information for *.xml file
			else if (strcmp(arg, "-X") == 0)
				usexml = TRUE;
			else
			assert_msg(FALSE, ("%s is an undefined option", arg));
		} else
			break;
		argv++;
		argc--;
	}

	assert_msg(argc, ("infile required"));

	char *ifile = argv[0];
	assert_msg((argc == 2), ("single outfile required"));
	char *ofile = argv[1];
	fileset->add_file(ifile, ofile);

	if (GenerateGraph) {
		char *dot_name = New_filename(argv[0], ".dot");
		if ((dot_fp = fopen(dot_name, "w")) == NULL) {
			printf("Cannot open file : %s\n", dot_name);
			exit(1);
		}
		fprintf(dot_fp, "digraph G{\n");
	}

	if (statistics) {
		char *stat_name = New_filename(argv[0], ".stat");
		if ((stat_fp = fopen(stat_name, "w")) == NULL) {
			printf("Cannot open file : %s\n", stat_name);
			exit(1);
		}
		fprintf(stat_fp,
				"###statistics information of thread size and pslice###\n");
	}

	const char *test_file_name = New_filename(argv[0], "_node_num_test.txt");
	if ((test_fp = fopen(test_file_name, "w")) == NULL) {
		printf("Cannot open test file .\n");
		exit(1);
	}

	const char *loop_file_name = New_filename(argv[0], "_loop__test.txt");
	if ((loop_fp = fopen(loop_file_name, "w")) == NULL) {
		printf("Cannot open test file .\n");
		exit(1);
	}

	if (usexml) {
		char *xml_name = New_filename(argv[0], ".xml");
		outfile.open(xml_name);

		xmlfile = new XmlDocument;
		//xmlfile->setStyleSheet("");
		root = new XmlElement("static_statistics_information");
		xmlfile->setRootElement(root);
	}

	/*Process each input file */
	file_set_entry *fse;
	fileset->reset_iter();
	while ((fse = fileset->next_file())) {
		ifile = fse->name();
		debug(2, "Processing file %s", ifile);

		target_arch = new archinfo(fse);
		printf("\n%s, %s, %s, %s\n", target_arch->family(),
				target_arch->version(), target_arch->vendor_os(),
				target_arch->implementation());
		assert(target_arch->family() == k_mips);
		target_regs = new reginfo(target_arch->fopen_mdfile("reg"));

		debug(3, "Processing procedures for %s", ifile);
		Process_file(fse);
		delete target_regs;
		delete target_arch;
	}
	delete fileset;

	if (GenerateGraph) {
		fprintf(dot_fp, "}\n");
		fclose(dot_fp);
	}
	fclose(test_fp);
	fclose(loop_fp);

	if (usexml) {
		char value[33];
		double average_thread_size = (double) (sum_of_thread_size
				+ sum_of_loop_size) / (num_of_threads + num_of_loops);
		/*printf("#############################%d, %d, %d, %d\n",
		 sum_of_thread_size, sum_of_loop_size, num_of_threads,
		 num_of_loops);*/
		sprintf(value, "%f", average_thread_size);
		subnode = new XmlElement("average_thread_size", std::string(value));
		root->addElement(subnode);

		double percent_of_nonloop_thread = (double) sum_of_thread_size
				/ (sum_of_thread_size + sum_of_loop_size);
		sprintf(value, "%f", percent_of_nonloop_thread);
		subnode = new XmlElement("percent_of_nonloop_thread",
				std::string(value));
		root->addElement(subnode);

		double percent_of_loop_thread = (double) sum_of_loop_size
				/ (sum_of_thread_size + sum_of_loop_size);
		sprintf(value, "%f", percent_of_loop_thread);
		subnode = new XmlElement("percent_of_loop_thread", std::string(value));
		root->addElement(subnode);

		/*printf("##############################%d, %d\n", pslice_of_thread,
		 pslice_of_loop);*/
		double average_pslice_size =
				(double) (pslice_of_thread + pslice_of_loop)
						/ (num_of_threads + num_of_loops);
		sprintf(value, "%f", average_pslice_size);
		subnode = new XmlElement("average_pslice_size", std::string(value));
		root->addElement(subnode);

		double percent_of_pslice_in_thread = average_pslice_size
				/ average_thread_size;
		sprintf(value, "%f", percent_of_pslice_in_thread);
		subnode = new XmlElement("percent_of_pslice_in_thread",
				std::string(value));
		root->addElement(subnode);

		double average_liveins = average_pslice_size - 1;
		sprintf(value, "%f", average_liveins);
		subnode = new XmlElement("arverage_liveins", std::string(value));
		root->addElement(subnode);

		outfile << xmlfile->toString();
	}
	//    exit_dfa();
	//    exit_cfg();
	//    exit_machine();
	//    exit_suif();
	cout<<"finished"<<endl;
	return 0;
}

/*
 *init_mpartition() -- Stuff that should be done before procedure partition.
 */
static void init_mpartition() {
	ANNOTE(k_cqip_pos, "cqip_pos", TRUE);
	ANNOTE(k_instr_profile, "instr_profile", TRUE);
	ANNOTE(k_pslice_entry_pos, "pslice_entry_pos", TRUE);
	ANNOTE(k_pslice_exit_pos, "pslice_exit_pos", TRUE);
	ANNOTE(k_proc_profile, "proc_profile", TRUE);
	ANNOTE(k_call_overhead, "call_overhead", TRUE);
	ANNOTE(k_looppos, "looppos", TRUE);
}

/*
 *Process_file() -- reads input SUIF file processing at one procedure at a time.
 */
static void Process_file(file_set_entry * fse) {
	proc_sym *cur_psym = NULL;
	fse->reset_proc_iter();
	while ((cur_psym = fse->next_proc())) {
		debug(3, "Processing procedure %s", cur_psym->name());
		printf("%s\n", cur_psym->name());

		/*read procedure with expression trees */
		cur_psymtab = Read_machine_proc(cur_psym, TRUE, FALSE);


		threshold::parse_file();
			threshold::print();
		Process_procedure(cur_psym);

		/*write procedure with expressioni trees */
		Write_machine_proc(cur_psym, fse);
		delete cur_psymtab;
	}
}

static void Process_procedure(proc_sym *cur_psym) {
	/*deal with procedure symbol ; make sure its legal*/
	Process_symtab(cur_psym->block()->proc_syms(), TRUE, stdout);
	/*init procedure cfg which contained data flow analysis */
	the_cfg = new da_cfg(cur_psym->block(), TRUE, TRUE, TRUE);
	the_cfg->remove_unreachable_blocks();
	the_cfg->find_dominators();
	the_cfg->find_postdominators();
	the_cfg->find_natural_loops();
	the_cfg->remove_shadows();
	/*init procedure cfg_node_list */
	the_full_cnl = the_cfg->reverse_postorder_list(); //深度优先排序，加快迭代速度

	the_bit_mgr = new operand_bit_manager(NULL, NULL, 1024, TRUE);
	enroll_variables(cur_psym->block(), the_bit_mgr); //登记变量
	the_teller = new reg_def_teller(cur_psym, the_bit_mgr); /*contain local variable */
	the_reach = new reaching_def_problem(the_cfg, the_teller);
	/*print verbose information of bit manager */

	if (verbose) {
		printf("\n###the information of bit manager###\n");
		print_bit_man(the_bit_mgr);
	}
	the_cfg->find_live_vars_more(the_bit_mgr);

	// \brief : print control flow graph and data flow analysis before partition
	if (verbose) {
		printf("************Before partition************\n\n");
		printf("###control flow graph begin###\n\n");
		the_cfg->print_cfg();
		printf("###control flow graph end###\n\n");
		printf("###data flow analysis begin###\n\n");
		the_cfg->print_dfa();
		printf("\n");
		printf("###data flow analysis end###\n\n");
	}
	// \brief : generate dot file to form *.png picture
	if (GenerateGraph) {
		the_cfg->generate_dot(dot_fp);

		//addByKeyming 20140513
		the_cfg->generate_dot(cur_psym->name(), da_cfg::BEFORE);
	}

	//print loop_depth
	/*  for(int i = 0; i < the_cfg->num_nodes(); ++i){
	 int depth = the_cfg->loop_depth(i);
	 boolean bLoopBegin = the_cfg->is_loop_begin(i);
	 boolean bLoopEnd = the_cfg->is_loop_exit(i);

	 printf("Node_%d depth is %d; begin:%d  exit %d\n", i, depth, bLoopBegin, bLoopEnd);
	 }
	 */
	fprintf(loop_fp, "\n当前访问函数:%s:\n", cur_psym->name());

	/*partition loops and construct super block cfg */
	threads = new std::vector<thread*>();
	loops = new std::vector<loop_block*>();
	the_scfg = construct_super_blocks();

	if (verbose) {
		print_all_definee(stdout, the_reach, the_scfg);
	}
	//------test for min_cut.h --------------------

	fprintf(test_fp, "\n当前访问函数:%s:\n", cur_psym->name());

	/**
	 * addbykeyming 10140528
	 */
	the_scfg->print_dom_block_list();
	non_loop_partition(the_scfg);

	/*对整个程序的超级块进行划分 */
	/* super_block *entry_block = the_scfg->entry();
	 super_block *start_block = entry_block->fall_succ();
	 super_block *end_block = the_scfg->exit();
	 thread *curr_thread = create_new_thread(start_block);
	 curr_thread = partition_thread(start_block, end_block, curr_thread);*/
	//the_cfg->print_cfg();

	/*write partition information */
	write_spawn_info();
	if (verbose) {
		printf("\n*********After partition************\n\n");
		the_cfg->print_cfg();
	}

	if (GenerateGraph) {
		//addByKeyming 20140513
		the_cfg->generate_dot(cur_psym->name(), da_cfg::AFTER);
	}

	/*clean up */
	delete the_reach;
	delete the_teller;
	delete the_bit_mgr;
	delete the_full_cnl;
	delete the_cfg;
	delete the_scfg;
	the_cfg = NULL;
	the_scfg = NULL;

}

/*
 *construct_super_blocks() --- construct super blocks according to the control flow graph, and when meet loop region, partition 
 *                              loop region.
 */
static super_block_cfg *construct_super_blocks() {
	super_block_cfg *scfg = new super_block_cfg();

	for (int i = 0; i < the_cfg->num_nodes(); i++) {
		if (the_cfg->is_loop_begin(i)) //If cfg_block is loop begin
				{
			//partition loop region, then set the loop region to one super block
			loop_block *loop = partition_loop(the_cfg->node(i));
			loop->set_block_num(i);
			scfg->append(loop);
			cfg_node *succ_node = loop->find_succ_node(the_cfg);
			if (!succ_node) {
				assert(0);
			} else {
				i = succ_node->number() - 1;
			}
		}
		//If cfg_block is non-loop region cfg_block
		else {
			cfg_node *node = the_cfg->node(i);
			super_block *sblock;
			if (node->is_begin()) {
				sblock = new super_block(super_block::CFG_BEGIN);
				scfg->set_entry(sblock);
			} else if (node->is_end()) {
				sblock = new super_block(super_block::CFG_END);
				scfg->set_exit(sblock);
			} else
				sblock = new super_block();

			sblock->set_block_num(i);
			sblock->add_block(node);
			scfg->append(sblock);
			/*	printf("scfg:");
			 scfg->print_relationship(stdout);
			 printf("\n");*/
		}
	}

	construct_super_block_relationship(scfg);
	//scfg->print_relationship();

	/**
	 * addbykeyming 20140528
	 */
	scfg->find_dom_block_list();
	return scfg;
}

/*
 *construct_super_block_relationship() --- construct super block relationship and super block's control flow graph.
 */
static void construct_super_block_relationship(super_block_cfg * sbc) {
	super_block_list_iter iter(sbc->super_blocks());
	while (!iter.is_empty()) {
		super_block *sb = iter.step();
		//LOOP:
		if (sb->knd() == super_block::LOOP) {
			loop_block *loop = (loop_block *) sb;
			cfg_node *entry_node = loop->entry_node();
			assert(entry_node != NULL);

			cfg_node *immed_dom_node = the_cfg->immed_dom(entry_node);

			if (immed_dom_node == 0)
				sb->set_immed_dom(0);
			else {
				loop->set_immed_dom(sbc->in_which_super_block(immed_dom_node));
			}
			cfg_node *succ_node = loop->find_succ_node(the_cfg);
			assert(succ_node != 0);
			super_block *succsb = sbc->in_which_super_block(succ_node);
			sb->set_immed_pdom(succsb);
			sb->set_fall_succ(succsb);
			sb->set_take_succ(0, 0.0);
		}
		//nLOOP
		else {
			cfg_node *node = peek_node(sb);
			assert(node != 0);

			cfg_node *immed_dom_node = the_cfg->immed_dom(node);

			if (immed_dom_node == 0)
				sb->set_immed_dom(0);
			else {
				sb->set_immed_dom(sbc->in_which_super_block(immed_dom_node));
			}
			cfg_node *immed_pdom_node = the_cfg->immed_postdom(node);

			if (immed_pdom_node == 0)
				sb->set_immed_pdom(0);
			else {
				sb->set_immed_pdom(sbc->in_which_super_block(immed_pdom_node));
			}

			if (node->is_end())
				sb->set_fall_succ(0);
			else {
				cfg_node *fall_node = node->fall_succ();

				if (fall_node == 0)
					sb->set_fall_succ(0);
				else {
					sb->set_fall_succ(sbc->in_which_super_block(fall_node));
				}

				if (!node->is_block()) {
					sb->set_take_succ(0, 0.0);
				} else {
					cfg_block *block = (cfg_block *) node;

					if (!block->ends_in_cbr())
						sb->set_take_succ(0, 0.0);
					else {
						cfg_node *taken_node = block->take_succ();
						if (taken_node == 0)
							sb->set_take_succ(0, 0.0);
						else {
							double taken_prob = branch_taken_probability(node);
							sb->set_take_succ(
									sbc->in_which_super_block(taken_node),
									taken_prob);
						}
					}
				}
			}
		}
	} //end of while
}

/*
 *peek_node() --- return cfg node which contained by super block.
 */
static cfg_node *peek_node(super_block * sb) {
	cfg_node_list_iter iter(sb->nodes());
	if (!iter.is_empty())
		return iter.step();
	else
		return 0;
}

/*
 *write_spawn_info() --- write spawn information to thread, the information contains 
 */
static void write_spawn_info() {
	std::vector<thread *>::iterator iter;
	for (iter = threads->begin(); iter != threads->end(); iter++) {
		thread *curr_thread = *iter;

		//write spawn information, insert spawn instruction and pre-compute slice
		//插入sp指令和预计算片段
		curr_thread->write_spawn_info(); //写当前线程的sp信息

		//statistics information
		num_of_threads++;
		sum_of_thread_size += curr_thread->static_size();

		if (statistics) {
			/*
			 if(curr_thread->get_thread_type() == thread::NONSPECULATIVE)
			 fprintf(stat_fp, "nonspeculative thread size : \t%d\n", curr_thread->size());
			 else if(curr_thread->get_thread_type() == thread::SPECULATIVE)
			 fprintf(stat_fp, "speculative thread size : \t%d\n", curr_thread->size());
			 */
			//如果当前线程的cqip点是sp所在块的后向支配节点，则打印为非推测线程
			if (the_cfg->postdominates(curr_thread->get_cqip_block(),
					curr_thread->get_spawned_block()))
				fprintf(stat_fp, "nonspeculative thread size: \t%d\n",
						curr_thread->static_size());
			else
				fprintf(stat_fp, "speculative thread size: \t%d\n",
						curr_thread->static_size());

			//fprintf(stat_fp, "pslice size: \t%d\n", curr_thread->get_pslice()->count());
		}
		if (usexml) {
			if (the_cfg->postdominates(curr_thread->get_cqip_block(),
					curr_thread->get_spawned_block()))
				//fprintf(stat_fp, "nonspeculative thread size: \t%d\n", curr_thread->size());
				curr_thread_element = new XmlElement(
						"nonspculative_thread_size", curr_thread->static_size());
			else
				//fprintf(stat_fp, "speculative thread size: \t%d\n", curr_thread->size());
				curr_thread_element = new XmlElement("speculative_thread_size",
						curr_thread->static_size());

			curr_thread_element->addElement(pslice_element);
			root->addElement(curr_thread_element);
		}
		delete curr_thread;
	}
	threads->clear();
	delete threads;
	threads = NULL;

	std::vector<loop_block *>::iterator loop_iter;
	for (loop_iter = loops->begin(); loop_iter != loops->end(); loop_iter++) {
		loop_block *loop = *loop_iter;
		//write spawn information, insert spawn, cancel instruction and pre-compute slice
		loop->write_spawn_info();

		//statistics
		num_of_loops++;
		sum_of_loop_size += loop->static_size();

		if (statistics) {
			//fprintf(stat_fp, "loop thread size : \t%d\n", loop->static_size());
			//fprintf(stat_fp, "pslice size: \t%d\n", loop->get_pslice()->count());
		}
		if (usexml) {
			loop_element = new XmlElement("loop_thread_size", loop->static_size());

			loop_element->addElement(pslice_element);
			root->addElement(loop_element);
		}
	}
	loops->clear();
	delete loops;
}

/*
 *New_filename() --- generates an output file name from an input filename and an extension.
 *                  return ofile pointer.
 */
static char *New_filename(char *ifile, char *ext) {
	int i;
	char *ofile = new char[strlen(ifile) + strlen(ext) + 1];

	i = strlen(ifile) - 1;
	while ((i > 0) && (ifile[i] != '.'))
		i--;
	strncpy(ofile, ifile, i);
	ofile[i] = '\0';
	strcat(ofile, ext);
	return ofile;
}

/*
 * 2011-4-25 注释 by SYJ
 * 递归遍历，检测符号表中sym_node类型是否合法。
 *
 */
static void Process_symtab(base_symtab * st, boolean descend, FILE * o_fd) {
	sym_node_list_iter sym_iter(st->symbols());
	while (!sym_iter.is_empty()) {
		sym_node *sn = sym_iter.step();
		if (sn->is_var()) {
			var_sym *v = (var_sym *) sn;
			if (v->is_extern()) {
				assert(v->is_static());
				assert(!v->has_var_def());
			} else if (v->has_var_def()) {
			} else {
				assert(v->is_auto());
			}
		} else if (sn->is_proc()) {
			proc_sym *p = (proc_sym *) sn;
		} else if (sn->is_label()) {
		} else
		assert_msg(FALSE, ("Process_symtab: unknown symbol type"));
	}

	if (descend) {
		base_symtab_list_iter child_iter(st->children());
		while (!child_iter.is_empty()) {
			base_symtab *child = child_iter.step();
			Process_symtab(child, TRUE, o_fd);
		}
	}
}
