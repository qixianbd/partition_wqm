#include "thread.h"
#include "spmt_instr.h"
#include "XmlElement.h"

#include "min_cut_fun.h"  //for  get_super_block_path_by_num_vec

extern boolean usexml;
extern XmlElement *pslice_element;
extern int pslice_of_thread;
/*
 *thread::thread() --- constructor of thread.
 */
thread::thread() : s_count(0)
{
    spawned_pos = 0;
    spawned_block = 0;
    pslice = 0;
    cqip_pos_num = 0;
    cqip_pos = 0;
    cqip_block = 0;
}

/*
 *thread::~thread() --- destructor of thread.
 */
thread::~thread()
{
}

/*
 *thread::set_spawn_count() --- set the spawn count
 */
void thread::set_spawn_count(int n)
{
    this->s_count = n;
}

/*
 *thread::spawn_count() --- return spawn count 
 */
int thread::spawn_count()
{
    return this->s_count;
}

/*
 *thread::inc_spawn_count() --- increase spawn count by one.
 */
void thread::inc_spawn_count()
{
    this->s_count++;
}

/*
 *thread::set_spawn_info() --- set spawn information.
 */
void thread::set_spawn_info(cfg_block *spawn_block, tree_node_list *pslice, label_sym *cqip_pos_num)
{
    this->spawned_block = spawn_block;
    this->pslice = pslice;
    this->cqip_pos_num = cqip_pos_num;
}

/*
 *thread::set_spawned_pos() --- set thread spawn position
 */
void thread::set_spawned_pos(tnle *spawn_pos)
{
    this->spawned_pos = spawn_pos;
}

/*
 *thread::get_spawned_pos() --- get thread spawn position获得线程的激发位置
 */
tnle* thread::get_spawned_pos()
{
    return spawned_pos;
}

/*
 *
 */
void thread::set_cqip_pos(tnle *cqip_pos)
{
    this->cqip_pos = cqip_pos;
}

/*
 *
 */
tnle* thread::get_cqip_pos()
{
    return cqip_pos;
}

/*
 *
 */
cfg_block* thread::get_spawned_block()
{
    return spawned_block;
}

/*
 *获取含有cqip点的块
 */
cfg_block* thread::get_cqip_block()
{
    return  cqip_block;
}

/*
 *thread::add_spawn_pos() --- add thread spawn position, to aim at multi-spawn
 */
void thread::add_spawn_pos(tnle *spawn_pos)
{
    spawn_poss.push_back(spawn_pos);
    ++s_count;
}

/*
 *thread::reach_any_spawn_pos() --- If tnle(spawn position) 
 */
bool thread::reach_any_spawn_pos(tnle * spawn_pos)
{
    std::vector<tnle *>::iterator iter;
    for(iter = spawn_poss.begin(); iter != spawn_poss.end(); iter++)
    {
        if(spawn_pos == *iter)
        {
            return true;
        }
    }
    return false;
}

void thread::print_block_list_num(FILE *fp)
{
	super_block_list_iter iter(this->super_blocks());
	fprintf(fp, "[");
	while(!iter.is_empty())
	{
		super_block *sblock = iter.step();
		fprintf(fp, "%d, ", sblock->block_num());
	}
	fprintf(fp, "]\n");
	fflush(fp);
}


/**
 * @description construct a thread, which contains all the super_blocks in pNode and has setted spawn_pos
 */
thread* thread::construct_thread_by_node_group(util::node *pNode, const super_block_cfg *scfg)
{
	assert(NULL != pNode);
	std::vector<int> num_vec = pNode->getNode_group();
	tnle *t_e = pNode->getSpawn_pos();
	super_block_path *path = get_super_block_path_by_num_vec(num_vec, scfg);

	thread *t = new thread();
	t->add_super_blocks(path->super_blocks());
	t->set_spawned_pos(t_e);
	return t;
}


/**
 * 由于循环内部节点不被scfg收录和管理，所以我勉强建立的super_block没有边之间的关系，还不如将其放在一个super_block中
 */
thread* thread::construct_loop_body_thread_by_group(util::node *pNode,  da_cfg *cfg){
	assert(NULL != pNode);
	std::vector<int> num_vec = pNode->getNode_group();
	tnle *t_e = pNode->getSpawn_pos();

	super_block *sb = get_super_block_list_by_vec(num_vec, cfg);

	thread *t = new thread();
	t->add_super_block(sb);
	t->set_spawned_pos(t_e);
	return t;
}

/*
 *thread::write_spawn_info() --- write spawn information for thread, such as: pre-compute slice, and spawn information
 线程激发写信息例如：预计算片段和激发点信息*/
void thread::write_spawn_info()
{
//    if(spawned_pos == 0)
//        return;
//    spawned_block->insert_after(pslice, spawned_pos);
    //
//    spawned_block->insert_after(pslice, cqip_pos);
//    insert_spawn_instr(spawned_block, spawned_pos, cqip_pos_num);
    if(spawned_pos == 0 || cqip_pos == 0)//sp或是cqip中有一个为空就返回
        return;
        //assert_msg(FALSE, ("cqip point is null"));
//    if(((instruction *)(((tree_instr *)spawned_pos->contents)->instr()))->opcode() == (int)mo_cqip)
//        return;
    if(statistics)
        fprintf(stat_fp, "pslice size: \t%d\t\t", pslice->count());

    if(usexml)
    {
        pslice_element = new XmlElement("pslice_size", pslice->count());
        pslice_of_thread += pslice->count();
    }

   // printf("pslice:\n");
    tree_node_list_iter iter(pslice);
  /*  while(!iter.is_empty())
    {
        tree_node *tn = iter.step();
        ((instruction *)((tree_instr *)tn)->instr())->print();
    }
    printf("\n");
*/
    /**
     * 当pslice为空时，此时只含有两条指令，一条pslice_entry和一条pslice_exit
     * 这时可以不用插入
     */

    printf("cqip_pos->next() is:\n");
    ((instruction *)((tree_instr *)(cqip_pos->next())->contents)->instr())->print();
    printf("\n");

    printf("pslice is:\n");
    pslice->print(stdout);
    printf("\n");

   	cqip_block->insert_after(pslice, cqip_pos->next());

    insert_spawn_instr(spawned_block, spawned_pos, cqip_pos_num);
    
   /* printf("cqip:\n");
    ((instruction *)((tree_instr *)(this->cqip_pos)->contents)->instr())->print();
    printf("\n");*/

   /* printf("spawn:\n");
    ((instruction *)((tree_instr *)(this->spawned_pos)->contents)->instr())->print();
    printf("\n");
    
    printf("label:\n");
    cqip_pos_num->print(stdout);
    printf("\n");*/
}
