#include <suif_copyright.h>
#include <string.h>
#include "common.h"

/*
 *insert_spawn_instr() --- insert spawn point instruction插入sp点指令
 */
tnle *insert_spawn_instr(cfg_block *sp_block, tnle *sp_pos, label_sym *cqip_pos) {
	immed_list *an_iml = new immed_list;
	an_iml->append(immed(cqip_pos));

	machine_instr *spawn_mi = new mi_bj(mo_spawn, cqip_pos);
	spawn_mi->set_annote(k_cqip_pos, an_iml);

	tree_node_list_e *new_tnle = new tree_node_list_e(new tree_instr(spawn_mi));

	//for sp_block's first tnle is cqip instr
	tnle *first_tnle_of_block = sp_block->first_non_label();
	tree_instr *first_instr = (tree_instr *) first_tnle_of_block->contents;
	machine_instr *first_mi = (machine_instr *) first_instr->instr();

//#define KEYMING_DEBEG
#ifdef KEYMING_DEBEG
	char *opstring = first_mi->op_string();
	fprintf(stderr,first_mi->op_string());
	cfg_node_instr_iter cnii(sp_block);
	int num = sp_block->number();
	fprintf(stderr, "%d\n", num);

	fprintf(stderr, "%s\n", ((tree_instr *)sp_pos->contents)->instr()->op_string());
	while(!cnii.is_empty()){
		tree_instr *ins = cnii.step();
		/*
		machine_instr *mi_in_block = (machine_instr *)(cnii.step()->instr());
		fprintf(stderr, "%d\t\t%s\n", mi_in_block->opcode(),  mi_in_block->op_string());
		mi_in_block->print(stderr);
		*/
		//ins->print(stderr);
	}
#endif
	/**
	 * 目的是想将spawn点插在cqip点之后，使得能够顺序激发
	 */
	if ((int)first_mi->opcode() == mo_cqip) {
		while ((int)first_mi->opcode() != mo_pslice_exit) {
			first_tnle_of_block = first_tnle_of_block->next();
			first_instr = (tree_instr *) first_tnle_of_block->contents;
			first_mi = (machine_instr *) first_instr->instr();
			printf("first_mi:\n");
			first_mi->print(stdout);
			printf("\n");
		}
		sp_pos = first_tnle_of_block->next();

	}

/**
 * addbykeyming 20140517
 * 判断sp_pos指向的位置是不是一条机器指令, 如果是机器指令
 * 判断该指令是否是loopbegin, 如果是loopbegin, 则将spawn 指令插入到loopbegin指令之后.
 * 下面该代码段的作用主要是保证 loopbegin  插入在spawn 之前.
 * 如果不加下面的代码段, 则对于循环部分, 很多(比如em3d.compute_note中)spawn 指令刚好插入在loopbegin前.
 */
	machine_instr *pos_instr_mi = dynamic_cast<machine_instr*> ((((tree_instr *)(sp_pos->contents))->instr()));
	if(pos_instr_mi != NULL){
		if((int)pos_instr_mi->opcode() == mo_loopbegin){
			fprintf(stderr, "insert after loopbegin");
			sp_block->insert_after(new_tnle, sp_pos);
			return new_tnle;
		}
	}
	//sp_block->insert_after(new_tnle, sp_pos);

	sp_block->insert_before(new_tnle, sp_pos);
	return new_tnle;
}

/*
 *insert_cqip_instr() --- insert control quasi-independent point instruction
 插入控制无关点指令
 */
tnle *insert_cqip_instr(cfg_block *cqip_block, bool before_label)
{
    assert(cqip_block->is_block());

    //label_sym *cqip_pos = cqip_block->get_label();
    label_sym *cqip_pos = cur_psymtab->pst()->new_unique_label();
    mi_lab *mi = new mi_lab(mo_lab, cqip_pos);

    immed_list *an_iml = new immed_list;
    an_iml->append(immed(cqip_pos));

    mi_bj *cqip_mi = new mi_bj(mo_cqip, cqip_pos);
    cqip_mi->set_annote(k_cqip_pos, an_iml);
    

    
    tree_node_list_e *new_tnle = new tree_node_list_e(new tree_instr(cqip_mi));
    if(before_label){
        cqip_block->push(new_tnle);
    }else{
    	cqip_block->insert_before(new_tnle, cqip_block->first_non_label());
    }
    tree_node_list_e *new_tnle_label = new tree_node_list_e(new tree_instr(mi));
    cqip_block->insert_after(new_tnle_label, new_tnle);

    return new_tnle;
}


/**
 * 这次循环的cqip点不再以基本块为边界，我们将cqip指令在pos前面插入
 */
tnle *insert_cqip_instr_for_loop(cfg_block *cqip_block, tnle *pos, bool before_label)
{
    assert(cqip_block->is_block());
    assert(pos != NULL);
    assert(cqip_block->contains(pos));

    label_sym *cqip_pos = cur_psymtab->pst()->new_unique_label();
    mi_lab *mi = new mi_lab(mo_lab, cqip_pos);

    immed_list *an_iml = new immed_list;
    an_iml->append(immed(cqip_pos));

    mi_bj *cqip_mi = new mi_bj(mo_cqip, cqip_pos);
    cqip_mi->set_annote(k_cqip_pos, an_iml);

    tree_node_list_e *new_tnle = new tree_node_list_e(new tree_instr(cqip_mi));
    if(cqip_block->last_non_cti())//当cqip_block只含有一条cti指令时返回NULL
    {
        //cqip_block->insert_after(new_tnle, cqip_block->last_non_cti());
        cqip_block->insert_before(new_tnle, pos);

        tree_node_list_e *new_tnle_label = new tree_node_list_e(new tree_instr(mi));
        cqip_block->insert_after(new_tnle_label, new_tnle);

        return new_tnle;
    }

    return 0;
}


/**
 * 由于不再在边界上面处理，所以需要修改此函数
 */
/*tnle *insert_cqip_instr_for_loop(cfg_block *cqip_block, bool before_label)
{
    assert(cqip_block->is_block());

    label_sym *cqip_pos = cur_psymtab->pst()->new_unique_label();
    mi_lab *mi = new mi_lab(mo_lab, cqip_pos);

    immed_list *an_iml = new immed_list;
    an_iml->append(immed(cqip_pos));

    mi_bj *cqip_mi = new mi_bj(mo_cqip, cqip_pos);
    cqip_mi->set_annote(k_cqip_pos, an_iml);

    tree_node_list_e *new_tnle = new tree_node_list_e(new tree_instr(cqip_mi));
    if(cqip_block->last_non_cti())
    {
        //cqip_block->insert_after(new_tnle, cqip_block->last_non_cti());
        cqip_block->insert_before(new_tnle, cqip_block->last_non_cti());

        tree_node_list_e *new_tnle_label = new tree_node_list_e(new tree_instr(mi));
        cqip_block->insert_after(new_tnle_label, new_tnle);

        return new_tnle;
    }

    return 0;
}*/
/*
 *insert_cancel_instr() --- insert cancel instruction
 插入cancel指令
 */
tnle *insert_cancel_instr(cfg_node *rarely, label_sym *cqip_pos)
{
    assert(rarely->is_block());

    immed_list *an_iml = new immed_list;
    an_iml->append(immed(cqip_pos));

    machine_instr *cancel_mi = new mi_bj(mo_cancel, cqip_pos);
//    machine_instr *squash_mi = new mi_bj(mo_squash, cqip_pos);
    tree_node_list_e *new_tnle = new tree_node_list_e(new tree_instr(cancel_mi));
    if(((cfg_block *)rarely)->first_active_op() != NULL)    //debug by voronoi
        ((cfg_block *)rarely)->insert_before(new_tnle, ((cfg_block *)rarely)->first_active_op());

    return new_tnle;
}

/*
 *peek_cqip_pos() --- look for control quasi-independent point position, and return the label symbol of cqip.
 */
label_sym *peek_cqip_pos(tnle *item)
{
    if(item->contents->is_instr())
    {
        tree_instr *ti = (tree_instr *)item->contents;
        immed_list *iml = (immed_list *)(ti->instr()->peek_annote(k_cqip_pos));
        if(iml)
        {
            return ((label_sym *)((*iml)[0].symbol()));
        }
    }

    return NULL;
}

/*
 *peek_pslice_entry_pos() --- look for entry position of pre-compute slice.
 寻找预计算片段的入口位置
 */
label_sym *peek_pslice_entry_pos(tnle *item)
{
    if(item->contents->is_instr())
    {
        tree_instr *ti = (tree_instr *)item->contents;
        immed_list *iml = (immed_list *)(ti->instr()->peek_annote(k_pslice_entry_pos));
        if(iml)
        {
            return ((label_sym *)((*iml)[0].symbol()));
        }
    }

    return NULL;
}

/*
 *peek_pslice_exit_pos() --- look for exit position of pre-compute slice.
 寻找预计算片段的结束位置
 */
label_sym *peek_pslice_exit_pos(tnle *item)
{
    if(item->contents->is_instr())
    {
        tree_instr *ti = (tree_instr *)item->contents;
        immed_list *iml = (immed_list *)(ti->instr()->peek_annote(k_pslice_exit_pos));
        if(iml)
        {
            return ((label_sym *)((*iml)[0].symbol()));
        }
    }

    return NULL;
}

/**
 * 比较语句
 * 具体指令可以参考machine/mips.data文件中对指令的定义
 */
bool is_cmp_op(instruction *i)
{
	/**
	 * seq, sge, sgeu, sgt, sgtu, sle, sleu, sne
	 */
	bool b1 =  (int) i->opcode() == mo_seq || (int) i->opcode() == mo_sge || (int) i->opcode() == mo_sgeu || (int) i->opcode() == mo_sgt
			|| (int) i->opcode() == mo_sgtu || (int) i->opcode() == mo_sle || (int) i->opcode() == mo_sleu || (int) i->opcode() == mo_sne;
	if(b1){
		return true;
	}

	/**
	 * slt, sltu, slti, sltiu,
	 */
	bool b2 =  (int) i->opcode() == mo_slt || (int) i->opcode() == mo_sltu || (int) i->opcode() == mo_slti || (int) i->opcode() == mo_sltiu;
	if(b2){
		return true;
	}

	/*
	 * c.seq.d, c.seq.s, c.le.d, c.le.s, c.lt.d, c.lt.s
	 */
	bool b3 = (int) i->opcode() == mo_c_seq_d || (int) i->opcode() == mo_c_seq_s || (int) i->opcode() == mo_c_le_d || (int) i->opcode() == mo_c_le_s
			||  (int) i->opcode() == mo_c_lt_d || (int) i->opcode() == mo_c_lt_s;
	if(b3){
		return true;
	}

	return false;
}


