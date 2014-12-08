/*  Control Flow Graph Node Implementation */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#define _MODULE_ "libcfg.a"

#pragma implementation "node.h"

#include <suif1.h>
#include <machine.h>
#include "cfg.h"


/* local function prototypes */
static void remove_node(cfg_node_list *l, cfg_node *n);
static void print_node_list(FILE *fp, cfg_node_list *l);


/* 
 * General constructor for cfg_node's.  The number and
 * parent should be overwritten when the cfg_node is 
 * attached to a cfg.  
 */
cfg_node::cfg_node ()
{
    set_number((unsigned)-1);
    set_parent(NULL);
    lpred = lsucc = NULL;
}


/* 
 * cfg_node::clear_layout_succ() -- Just manipulate the
 * layout pointers.  The actual work of re-inserting
 * shadow instructions is done in the subclass
 * methods.  
 */
void
cfg_node::clear_layout_succ ()
{ 
    if (lsucc) {
	assert(lsucc->lpred == this); 
	lsucc->lpred = NULL; 
	lsucc = NULL; 
    }
}


/* cfg_node::set_layout_succ() -- Just manipulate the
 * layout pointers.  The actual work of inverting polarity
 * and inserting shadow instructions is done in the
 * subclass methods.  
 * 
 * Always return FALSE because this routine never inverts
 * branch polarity. 
 */
boolean 
cfg_node::set_layout_succ (cfg_node *nod) 
{ 
    if (nod == NULL) {
	clear_layout_succ();
	return FALSE; 
    }
    assert(nod->lpred == NULL); 
    lsucc = nod; 
    lsucc->lpred = this; 
    return FALSE; 
}


/* 
 * cfg_node::set_layout_succ(label_sym) -- wrapper routine
 * that translates a label successor into the corresponding
 * cfg_block, then calls the other set_layout_succ() routine.
 */
boolean 
cfg_node::set_layout_succ (label_sym *lab)
{
    cfg_node *new_succ = label_cfg_node(lab); 
    assert_msg(new_succ, ("set_layout_succ() -- "
	"cannot find cfg_node for specified label")); 
    return set_layout_succ(new_succ); 
}


/*
 * cfg_node::insert_empty_block(n) -- create a block containing
 * a dummy label instruction, and insert the new block along the
 * n-th successor edge of the base block.
 * Blocks created along edges are often called ``landing pads''.
 * Landing pads are useful in instruction scheduling, where code
 * sometimes needs to be placed between a block and its predecessor,
 * rather than in either one.
 */

cfg_block * 
cfg_node::insert_empty_block(unsigned n)
{
    assert((int)n < succs()->count()); 
    cfg_node *dst = (*succs())[n]; 
    assert(dst->is_block()); 
    cfg_block *dest = (cfg_block *)dst; 

    proc_symtab *par_symtab = parent()->tproc()->proc_syms(); 
    label_sym *new_lab = par_symtab->new_unique_label(); 
    label_sym *dst_lab = dest->get_label(); 

    // Create and attach the block 
    tree_node_list *lp_tnl = new tree_node_list; 
    tree_instr *lp_lab, *lp_jmp; 
    lp_lab = new tree_instr(New_lab(new_lab)); 
    lp_jmp = new tree_instr(New_ubr(dst_lab)); 

    lp_tnl->append(lp_lab); 
    lp_tnl->append(lp_jmp); 
    cfg_block *lp_blk = new cfg_block(lp_tnl, lp_lab, lp_jmp); 
    lp_lab->annotes()->append(new annote(k_cfg_node, (void *)lp_blk)); 
    lp_jmp->annotes()->append(new annote(k_cfg_node, (void *)lp_blk)); 
    new_lab->annotes()->append(new annote(k_cfg_node, (void *)lp_blk));
    lp_blk->set_in_cti(lp_jmp); 
    parent()->attach(lp_blk); 

    // Link to the block
    assert_msg(is_block(), 
	("insert_empty_block: unsupported source node type"));
    boolean preserve_layout = (this->layout_succ() == dst); 
    if (preserve_layout) {
	this->clear_layout_succ(); 
    }
    set_succ(n, lp_blk); 
    lp_blk->set_succ(0, dest); 
    if (preserve_layout) {
	this->set_layout_succ(lp_blk); 
	lp_blk->set_layout_succ(dst); 
    }
    return lp_blk; 
}

cfg_block * 
cfg_node::insert_empty_block (cfg_node *dst)
{
    assert(occurs(dst, this->succs())); 
    assert(dst->is_block()); 
    cfg_block *dest = (cfg_block *)dst; 

    proc_symtab *par_symtab = parent()->tproc()->proc_syms(); 
    label_sym *new_lab = par_symtab->new_unique_label(); 
    label_sym *dst_lab = dest->get_label(); 

    // Create and attach the block 
    tree_node_list *lp_tnl = new tree_node_list; 
    tree_instr *lp_lab, *lp_jmp; 
    lp_lab = new tree_instr(new mi_lab(Label_op(target_arch), new_lab)); 
    lp_jmp = new tree_instr(new mi_bj(Ubr_op(target_arch), dst_lab)); 

    lp_tnl->append(lp_lab); 
    lp_tnl->append(lp_jmp); 
    cfg_block *lp_blk = new cfg_block(lp_tnl, lp_lab, lp_jmp); 
    lp_lab->annotes()->append(new annote(k_cfg_node, (void *)lp_blk)); 
    lp_jmp->annotes()->append(new annote(k_cfg_node, (void *)lp_blk)); 
    new_lab->annotes()->append(new annote(k_cfg_node, (void *)lp_blk));
    lp_blk->set_in_cti(lp_jmp); 
    parent()->attach(lp_blk); 

    // Link to the block
    assert_msg(is_block(), 
	("insert_empty_block: unsupported source node type"));
    boolean preserve_layout = (this->layout_succ() == dst); 
    if (preserve_layout) {
	this->clear_layout_succ(); 
    }
    cfg_node_list_iter cnli(succs()); 
    for (int i = 0; !cnli.is_empty(); ++i) {
	cfg_node *scan = cnli.step(); 
	if (scan == dst)
	    set_succ(i, lp_blk); 
    }
    lp_blk->set_succ(0, dest); 
    if (preserve_layout) {
	this->set_layout_succ(lp_blk); 
	lp_blk->set_layout_succ(dst); 
    }
    return lp_blk; 
}


/*
 * ---------- Implementation of CFG_BEGIN methods ----------
 */

/* 
 * cfg_begin::set_succ() -- Replace successor of the entry node.
 * Only the first successor may be replaced; the second must
 * point to the exit node.
 *
 * In machine-level code the entry point is marked by a
 * k_proc_entry annotation.  Code generators are expected to
 * place this annotation on the first instruction of the entry
 * block and printmachine is expected to handle the difference
 * between the _beginning_ of the procedure and the _entry point_
 * of the procedure appropriately.
 *
 * In machine code, find and remove the old k_proc_entry annote,
 * if any.  Then stick the new annotation on the new entry
 * point.
 */
void
cfg_begin::set_succ (unsigned n, cfg_node *nod)
{
    annote *an = NULL;
    assert(n == 0);
    assert(nod->is_block());
    cfg_block *old_succ, *new_succ = (cfg_block *)nod;

    old_succ = succs()->is_empty() ? NULL : (cfg_block *)(*succs())[0];
    
    if (target_arch->family() != k_suif) {
        cfg_node_instr_iter cnii(old_succ); 
        while (!an && !cnii.is_empty()) {
            tree_instr *ti = cnii.step();
            instruction *i = ti->instr();
            if (i->are_annotations())
                an = i->annotes()->get_annote(k_proc_entry);
        }
        if (old_succ)
            assert_msg(an, ("cfg_begin::set_succ() -- no k_proc_entry "
                            "annotation on old entry point"));
        else
            an = new annote(k_proc_entry);
        new_succ->in_head()->instr()->annotes()->append(an);        
    }

    if (old_succ)
	succs()->pop();
    succs()->push(nod);

    // Clean up pred pointers
    if (old_succ && !occurs(old_succ, succs()))
	remove_node(old_succ->preds(), this);
    if (!occurs(this, new_succ->preds()))
	new_succ->preds()->append(this);
}


/* 
 * cfg_begin::set_layout_succ() -- Move a block up to the front
 * of the SUIF tree_node_list.  
 *     We really care about the _clump_ that the target block
 * represents.  A clump is a set of blocks that have been joined together
 * by layout pointers (and are thus contiguous in the tree_node_list).  
 * The driver program has requested that nod's clump appear at the
 * beginning of the SUIF procedure.  
 *     Always returns FALSE because the cfg_begin node has no
 * control instruction to invert. 
 */
boolean
cfg_begin::set_layout_succ (cfg_node *bot)
{
    assert_msg(lpred == NULL, ("set_layout_succ() -- "
	"begin block has a layout predecessor")); 

    if (bot == NULL) {
	clear_layout_succ();
	return FALSE; 
    }

    assert(bot->is_block()); 
    cfg_block *bot_start = (cfg_block *)bot; 

    // Verify layout pointers OK, same tnl in blocks
    assert_msg(this->lsucc == NULL, ("set_layout_succ() -- "
	"begin block already has a layout successor")); 
    assert_msg(bot_start->layout_pred() == NULL, ("set_layout_succ() -- "
	"bottom block already has a layout predecessor")); 
    assert_msg(this->parent() == bot_start->parent(), 
	("set_layout_succ: requested blocks do not belong to the same cfg")); 

    // Check for layout cycles; find end of bottom clump
    cfg_block *bot_end = bot_start; 
    while (bot_end->layout_succ()) {
	bot_end = (cfg_block *)bot_end->layout_succ(); 
	assert(bot_end->is_block()); 
    }

    // Move instructions from bottom clump 
    tree_node_list_e *ins_point = NULL; 
    tree_node_list_e *del_point = bot_start->in_head()->list_e(); 
    tree_node_list_e *end_point = bot_end->in_shad() 
	? bot_end->in_shad()->list_e()->next()
	: bot_end->in_tail()->list_e()->next(); 
    tree_node_list_e *temp, *next; 
    tree_node_list *tnl = parent()->tproc()->body(); 
    do {
	tree_node_list_e *next = del_point->next(); 
	tree_node_list_e *temp = tnl->remove(del_point); 
	if (ins_point == NULL) 
	    tnl->push(temp); 
	else
	    tnl->insert_after(temp, ins_point); 
	del_point = next; 
	ins_point = temp; 
    } while (del_point != end_point); 

    // Link the lsucc and lpred fields. 
    cfg_node::set_layout_succ(bot); 

    return FALSE; 
}


/*
 * ---------- Implementation of CFG_BLOCK methods ----------
 */

/* This constructor creates an unattached copy of cfg_block orig.  This
 * routine needs to be fixed to work with low-suif mbr instructions. */
cfg_block::cfg_block (cfg_block *orig, base_symtab *dst_scope)
{
    int num_tgts = 0; 
    int entry_bits = target.size[C_ptr]; 
    var_sym *old_tab = NULL, *new_tab = NULL; 
    var_def *old_def = NULL, *new_def = NULL; 
    base_symtab *targ_scope = dst_scope; 
    if (targ_scope == NULL)
	targ_scope = orig->parent()->tproc()->proc_syms(); 


    // Clone the tree node list
    // Need to take special care with mbr jump tables
    replacements r; 
    assert(r.oldinstrs.count() == 0);
    if (orig->ends_in_mbr() 
	&& orig->in_cti()->instr()->architecture() != k_suif) {
	instruction *mbri = orig->in_cti()->instr(); 
	assert(mbri->architecture() != k_suif); 
	immed_list *il = (immed_list *)mbri->peek_annote(k_instr_mbr_tgts);
	num_tgts = (*il)[0].integer(); 
	assert((*il)[1].symbol()->is_var()); 
	old_tab = (var_sym *)(*il)[1].symbol(); 
	old_def = old_tab->definition(); 

	// Generate new type for dispatch table
	type_node *dtable_entry_type = type_ptr; 
	type_node *dtable_type = new array_type(dtable_entry_type, 
	    array_bound(0), array_bound(num_tgts - 1));
	dtable_type = targ_scope->install_type(dtable_type);

	// Install var_sym into target procedure symbol table 
	new_tab = targ_scope->new_unique_var(dtable_type, old_tab->name());
	new_tab->reset_userdef();        /* not user defined */

	// Build new, empty definition for new_tab
	new_def = targ_scope->define_var(new_tab, 32);
	for (int i = 0; i < num_tgts; i++) {
	    immed_list *il = new immed_list();
	    il->append(immed(1));                   // no. of copies 
	    il->append(immed(entry_bits));          // total bits in value 
	    il->append(immed((label_sym *)NULL));   // data value 
	    new_def->append_annote(k_repeat_init, il);
	}
    }
    if (orig->ends_in_mbr() 
	&& orig->in_cti()->instr()->architecture() == k_suif) {
	in_mbr *mbri = (in_mbr *)orig->in_cti()->instr(); 
	num_tgts = mbri->num_labs() + 1;  /* plus one is for default tgt */
    }

    cfg_node_instr_iter cnii(orig, /* reverse = */ TRUE);  // bug fix by tjc
    while (!cnii.is_empty()) {
	tree_instr *ti = cnii.step(); 
	ti->find_exposed_refs(dst_scope, &r); 
    }

    assert(r.oldinstrs.count() == 0);
    r.resolve_exposed_refs(dst_scope); 

    tiC = NULL; 
    tnl = new tree_node_list; 
    cfg_node_instr_iter cnii3(orig /* reverse = FALSE */); 
    while (!cnii3.is_empty()) {
	tree_instr *tiSrc = cnii3.step(); 
	tree_instr *tiDst = (tree_instr *)tiSrc->clone_helper(&r); 
	tiDst->instr()->get_annote(k_proc_entry);   // tjc
	delete tiDst->get_annote(k_cfg_node); 
	annote *an2 = new annote(k_cfg_node, (void *)this); 
	tiDst->annotes()->append(an2); 
	tnl->append(tiDst); 
	if (orig->tiC == tiSrc)
	    this->tiC =  tiDst; 
    }

    // Set ti1 and ti2
    ti1 = (tree_instr *)tnl->head()->contents; 
    ti2 = (tree_instr *)tnl->tail()->contents; 
    tiS = NULL; 

    // number set to -1, parent set to NULL by cfg_node::cfg_node()
    // prs, scs constructed by cfg_node::cfg_node()

    // If an mbr, set up edges to NULL.  And replace refs to old stuff
    if (ends_in_mbr()) {
	for (int i = 0; i < num_tgts; ++i)
	    succs()->append((cfg_node *)NULL); 

	if (orig->in_cti()->instr()->architecture() != k_suif) {
	    assert(old_tab && new_tab && old_def && new_def); 
	    replacements r2; 
	    r2.oldsyms.push(old_tab); 
	    r2.newsyms.push(new_tab); 
	    r2.olddefs.push(old_def); 
	    r2.newdefs.push(new_def); 

	    cfg_node_instr_iter cnii2(this); 
	    while (!cnii2.is_empty()) {
		tree_instr *ti = cnii2.step(); 
		ti->clone_helper(&r2, TRUE /* no_copy */); 
	    }
	}
    }
}


/* This constructor creates an unattached empty block. */
cfg_block::cfg_block (block_symtab *dst_scope)
{
    assert(dst_scope != NULL); 

    tnl = new tree_node_list; 
    label_sym *ls = dst_scope->new_unique_label(); 

    instruction *in; 
    in = New_lab(ls); 
    tnl->append(new tree_instr(in)); 

    ti1 = (tree_instr *)tnl->head()->contents; 
    ti2 = (tree_instr *)tnl->tail()->contents; 
    tiC = NULL; 
    tiS = NULL; 
}


/*
 * cfg_block::set_succ() -- Make suc the n-th successor of this basic
 * block.  If n is 0, be sure that the block's shadow slot correctly
 * reflects its CTI, it's zero-th successor, and its layout status.
 * (A block with a layout successor has no shadow goto.)
 */
void
cfg_block::set_succ (unsigned n, cfg_node *suc)
{
    cfg_node *old_succ = NULL;			// until proven otherwise

    if (n == 0 && tiS != NULL) {		// erase shadow now, if any ..
	delete tnl->remove(tiS->list_e());	// .. (but maybe set it below)
	delete tiS;
	tiS = NULL;
    }

    if (suc->is_end()) {
	assert(ends_in_return());

	if (!succs()->is_empty())
	    old_succ = succs()->pop(); 
	assert(succs()->is_empty()); 
	succs()->push(suc); 

    } else {
	assert(suc->is_block()); 
	cfg_block *new_succ = (cfg_block *)suc; 
	instruction *cti = in_cti() ? in_cti()->instr() : NULL; 

	if (!ends_in_mbr()) {

	    if (n == 0) {	// 0. fall-through  or  ubr target

		if (!succs()->is_empty())
		    old_succ = succs()->pop(); 
		succs()->push(new_succ); 

		if (!lsucc) {
		    set_shadow(suc);	// if ubr, it recedes into shadow
		    cti = NULL;		// either way, no new branch target

		} else if (!ends_in_ubr()) {
		    assert_msg(suc == lsucc,
			("set_succ() -- node has layout successor "
			 "but requested different fall-through successor")); 
		    cti = NULL;
		}

	    } else {		// 1. cbr target, taken case

		assert(n == 1 && ends_in_cbr()); 

		switch (succs()->count()) {
		case 0:
		    succs()->append((cfg_node *)NULL); 
		    succs()->append(new_succ); 
		    break; 
		case 1:
		    succs()->append(new_succ); 
		    break; 
		default:
		    old_succ = succs()->head()->next()->contents; 
		    succs()->head()->next()->contents = new_succ;
		    break; 
		}
	    }

	    if (cti) {			// ubr or cbr needs new target

		if (cti->architecture() == k_suif) {
		    ((in_bj *)cti)->set_target(new_succ->get_label()); 
		} else {
		    ((mi_bj *)cti)->set_target(new_succ->get_label());
		}
	    }
	    
	} else if (cti->architecture() != k_suif) {	// 2. mbr target

	    assert(ends_in_mbr()); 

	    immed_list *iml = (immed_list *)
		cti->peek_annote(k_instr_mbr_tgts);
	    assert_msg(iml, ("add_cfg_edge - no mbr targets"));
	    int num = (*iml)[0].integer();
	    assert((int)n < num); 
	    sym_node *dtable_sym = (*iml)[1].symbol(); 
	    assert(dtable_sym->is_var()); 
	    var_sym *dtable_var = (var_sym *)dtable_sym; 
	    var_def *dtable_def = dtable_var->definition(); 

	    unsigned j; 
	    if (succs()->count() == 0) 
		for (j = 0; j < (unsigned)num; ++j)
		    succs()->append(new cfg_node_list_e(NULL)); 
	    cfg_node_list_e *cnle = succs()->head(); 
	    immed_list **tgts = new immed_list*[num]; 
	    for (j = 0; j < (unsigned)num; j++) {
		tgts[j] = (immed_list *)dtable_def->get_annote(k_repeat_init); 
		assert(tgts[j]); 
		if (j == n) {
		    immed_list *il = new immed_list; 
		    il->append((*tgts[n])[0].integer()); 
		    il->append((*tgts[n])[1].integer()); 
		    il->append(new_succ->get_label()); 
		    delete tgts[n]; 
		    tgts[n] = il; 

		    cfg_node_list_e *new_elem = new cfg_node_list_e(new_succ);
		    if (cnle == NULL) {
			succs()->append(new_elem); 
		    } else {
			old_succ = cnle->contents; 
			succs()->insert_before(new_elem, cnle); 
			delete succs()->remove(cnle); 
		    }
		    cnle = new_elem; 
		}
		cnle = cnle->next(); 
	    }
	    for (j = 0; j < (unsigned)num; j++)
		dtable_def->append_annote(k_repeat_init, tgts[j]); 
	    delete tgts; 

	} else {	// 2. mbr target -- k_suif version

	    assert(ends_in_mbr()); 
	    assert(cti->opcode() == io_mbr);
	    in_mbr *mbri = (in_mbr *)cti;
	    unsigned num = mbri->num_labs();
	    if (n == num)	/* last successor is default target */
		mbri->set_default_lab(new_succ->get_label()); 
	    else
		mbri->set_label(n, new_succ->get_label()); 

	    // pad out succs to nth entry if necessary
	    unsigned j; 
	    for (j = succs()->count(); j <= n; ++j)
		succs()->append(new cfg_node_list_e(NULL)); 

	    // replace nth successor, remember old_succ
	    cfg_node_list_e *cnle = succs()->head(); 
	    for (j = 0; j < 1 + num; j++) {
		if (j == n) {
		    cfg_node_list_e *new_elem = new cfg_node_list_e(new_succ);
		    if (cnle == NULL) {
			succs()->append(new_elem); 
		    } else {
			old_succ = cnle->contents; 
			succs()->insert_before(new_elem, cnle); 
			delete succs()->remove(cnle); 
		    }
		    cnle = new_elem; 
		}
		cnle = cnle->next(); 
	    }

	}
    }
    // Clean up pred pointers
    if (old_succ && !occurs(old_succ, succs()))
	remove_node(old_succ->preds(), this); 
    if (!occurs(this, suc->preds()))
	suc->preds()->append(this);

    return; 
}

/* cfg_block::clear_layout_succ() -- Disconnect two blocks in the layout
 * chain.  This may involve demoting unconditional branches into 
 * shadows or inserting a new shadow goto instruction.
 * Just call set_shadow() to do the real work. 
 */
void
cfg_block::clear_layout_succ (void)
{
    cfg_node::clear_layout_succ(); 
    if (succs()->count())
	set_shadow(succs()->head()->contents); 
}


/* 
 * cfg_block::set_layout_succ() -- Connect two blocks in the layout
 * chain, and update the underlying SUIF tree_node_list so that these
 * two blocks are now adjacent.  
 *     We really care about the _clump_s that the two blocks
 * represent.  A clump is a set of blocks that have been joined together
 * by layout pointers (and are thus contiguous in the tree_node_list).  
 * In the procedure below, we will talk about the top block (this) and
 * the bottom block (bot); the driver program has requested that 
 * bot follow this in layout order.  
 *     The top block should be the last block in the top clump; the 
 * bottom block should be the first block in the bottom clump.  If not, 
 * then we are trying to link into the middle of a clump, which is an
 * error.  
 *     The top and bottom clump should be different.  If they are the
 * same clump, then setting the layout link will create a layout cycle, 
 * which is also an error.  
 *     We optimize control instructions when the layout successor
 * is set.  If the top block is an unconditional or conditional (2-way)
 * branch/jump and the bottom block is a graph successor of the top 
 * block, then we can make the top block fall through to the bottom
 * block on some path.  Doing this may require removing shadow instructions
 * and/or inverting the polarity of the top block's conditional branch. 
 *     Returns TRUE if this's conditional branch was inverted because
 * of this layout request.  
 */
boolean 
cfg_block::set_layout_succ (cfg_node *bot)
{
    boolean inverted = FALSE; 

    if (bot == NULL) {
	clear_layout_succ();
	return FALSE; 
    }

    assert(bot->is_block()); 

    cfg_block *bot_start = (cfg_block *)bot; 

    // Verify layout pointers OK, same tnl in blocks
    assert_msg(this != bot, 
	("set_layout_succ: cannot place a block after itself")); 
    assert_msg(this->lsucc == NULL, 
	("set_layout_succ: top block already has a layout successor")); 
    assert_msg(bot_start->lpred == NULL, 
	("set_layout_succ: bottom block already has a layout predecessor")); 
    assert_msg(this->tnl == bot_start->tnl, 
	("set_layout_succ: requested blocks do not belong to the same cfg")); 

    // Check for layout cycles; find end of bottom clump
    cfg_block *bot_end = bot_start; 
    while (bot_end->lsucc) {
	assert_msg(bot_end != this, ("set_layout_succ: layout cycle")); 
	bot_end = (cfg_block *)bot_end->lsucc; 
	assert(bot_end->is_block()); 
    }
    assert_msg(bot_end != this, ("set_layout_succ: layout cycle")); 

    // Check layout successor is a cbr control successor
    assert_msg(!ends_in_cbr()
	|| ((*succs())[0] == bot)
	|| ((*succs())[1] == bot), 
	("set_layout_succ() -- layout successor of a cbr block "
	"must be either the fall-through or taken successor")); 

    // Check layout successor is a call control successor 
    assert_msg(!ends_in_call()
	|| ((*succs())[0] == bot), 
	("set_layout_succ() -- layout successor of a call block "
	"must be the fall-through successor")); 

    // Fix up end of top clump (invert cbrs, remove shadows)
    if (ends_in_cbr() && succs()->count() > 1 && (*succs())[1] == bot_start) {
	invert_branch(); 
	inverted = TRUE; 

	// swap succs(0) and succs(1)
	cfg_block *new_targ = (cfg_block *)succs()->pop(); 
	cfg_block *new_fall = (cfg_block *)succs()->pop(); 
	assert(new_targ->is_block()); 
	assert(new_fall->is_block()); 
	succs()->push(new_targ); 
	succs()->push(new_fall); 

	// set the new branch target
	instruction *top_lasti = in_tail()->instr(); 
	if (top_lasti->architecture() == k_suif) {
	    ((in_bj *)top_lasti)->set_target(new_targ->get_label()); 
	} else {
	    ((mi_bj *)top_lasti)->set_target(new_targ->get_label());
	}
    }
    if (succs()->count()) {
	if ((*succs())[0] == bot_start || ends_in_cti()) {
	    set_shadow(NULL); 
	} else {
	    promote_shadow(); 
	}
    } 

    // Move instructions from bottom clump 
    tree_node_list_e *ins_point = this->tiS
	? this->tiS->list_e()
	: this->ti2->list_e(); 
    tree_node_list_e *del_point = bot_start->ti1->list_e(); 
    tree_node_list_e *end_point = bot_end->tiS 
	? bot_end->tiS->list_e()->next()
	: bot_end->ti2->list_e()->next(); 

    if (ins_point->next() != del_point)
	do {
	    tree_node_list_e *next = del_point->next(); 
	    tree_node_list_e *temp = tnl->remove(del_point); 
	    tnl->insert_after(temp, ins_point); 
	    del_point = next; 
	    ins_point = temp; 
	} while (del_point != end_point); 

    // Link the lsucc and lpred fields. 
    cfg_node::set_layout_succ(bot); 

    return inverted; 
}

/* 
 * invert_branch() -- Invert the branch by changing the opcode. 
 * This is usually used in conjunction with set_succ() to 
 * swap the targets of a conditional branch while maintaining
 * program correctness.
 * If the branch has edge frequencies, swap them.
 */

void 
cfg_block::invert_branch ()
{
    assert(tiC); 
    instruction *inC = tiC->instr();
    assert(Is_cbr(inC)); 

    int orig_op = inC->opcode(); 
    if_ops new_op = (if_ops)Invert_cbr_op(target_arch, orig_op);
    inC->set_opcode(new_op); 

    static char *k_branch_edge_weights =
	lexicon->enter("branch_edge_weights")->sp;

    immed_list *il = (immed_list *)inC->get_annote(k_branch_edge_weights);
    if (il) { 
	assert_msg(il->count() == 2,
		   ("cbr has %d branch edge weights, not 2", il->count()));
	il->append(il->pop());
	inC->append_annote(k_branch_edge_weights, il);
    }
}

/* 
 * promote_shadow() -- Promote a shadow ubr instruction to be the
 * terminating control instruction of this block.  
 * Assert fail if no shadow or there already is a terminating 
 * control instruction. 
 */
void
cfg_block::promote_shadow (void)
{
    assert_msg(tiS, ("Cannot promote non-existent shadow")); 
    assert_msg(!tiC, ("Cannot promote shadow when block already has cti"));
    assert(succs()->count() == 1); 

    ti2 = tiC = tiS; 
    tiS = NULL; 
}

/* 
 * set_shadow() -- if necessary, add a shadow instruction to a block.
 */
void 
cfg_block::set_shadow (cfg_node *nod)
{
    // Remove any old ubr, if any
    if (ends_in_ubr()) {
	assert_msg(ti1 != ti2, ("set_shadow() -- "
	    "Deleted old ubr is entire block")); 
	if (tiC == ti2) 
	    ti2 = (tree_instr *)ti2->list_e()->prev()->contents; 
	delete tnl->remove(tiC->list_e()); 
	delete tiC;
	tiC = NULL; 
    }

    // Remove the old shadow, if any
    if (tiS != NULL) {
	delete tnl->remove(tiS->list_e());
	delete tiS;
	tiS = NULL;
    }

    if (nod == NULL || ends_in_mbr() || ends_in_return())
	return; 

    assert(nod->is_block()); 
    cfg_block *new_succ = (cfg_block *)nod; 
    instruction *new_shadow;
    new_shadow = New_ubr(new_succ->get_label()); 
    tiS = new tree_instr(new_shadow); 
    tiS->annotes()->append(new annote(k_cfg_node, (void *)this));
    tnl->insert_after(tiS, in_tail()->list_e()); 
}

/* 
 * get_label() -- Find the label_sym that starts a cfg_block.
 * If there isn't one, add one and return it.  
 */
label_sym *
cfg_block::get_label (void)
{
    if (Is_label(in_head()->instr()))
	return Get_label(in_head()->instr());
    
    tree_proc *tp = parent()->tproc();
    proc_symtab *par_symtab = tp->proc_syms(); 
    label_sym *new_lab = par_symtab->new_unique_label(); 
    new_lab->annotes()->append(new annote(k_cfg_node, (void *)this));
    instruction *new_i; 
    new_i = New_lab(new_lab); 

    tree_instr *new_ti = new tree_instr(new_i); 
    new_ti->annotes()->append(new annote(k_cfg_node, (void *)this)); 
    tnl->insert_before(new_ti, in_head()->list_e()); 
    set_in_head(new_ti); 
    return new_lab; 
}


/* set of simple test routines */
boolean 
cfg_block::ends_in_ubr () const
{
    return tiC && Is_ubr(tiC->instr()); 
}

boolean 
cfg_block::ends_in_cbr () const
{
    return tiC && Is_cbr(tiC->instr()); 
}

boolean 
cfg_block::ends_in_mbr () const
{
    return tiC && Is_mbr(tiC->instr()); 
}

boolean 
cfg_block::ends_in_call () const
{
    return tiC && Is_call(tiC->instr()); 
}

boolean 
cfg_block::ends_in_return () const
{
    return tiC && Is_return(tiC->instr()); 
}


/* 
 * cfg_block::first_non_label() -- returns pointer to the first
 * tnl element that is not a label.  Routine returns NULL if the
 * block is all label instructions. 
 */
tnle *
cfg_block::first_non_label (void) const
{
    cfg_node_instr_iter instrs((cfg_node *)this);
    while (!instrs.is_empty()) {
	tree_instr *ti = instrs.step();
	if (!Is_label(ti->instr()))
	    return ti->list_e();
    }
    return NULL; // block is all labels
}

/* 
 * cfg_block::first_active_op() -- returns pointer to the first
 * tnl element that is not a label, a line directive, or a null
 * instruction.  Routine returns NULL if the block contains only
 * inactive operations of these kinds. 
 */
tnle *
cfg_block::first_active_op (void) const
{
    cfg_node_instr_iter instrs((cfg_node *)this);
    while (!instrs.is_empty()) {
	tree_instr *ti = instrs.step();
	instruction *i = ti->instr();
	if (!Is_label(i) && !Is_line(i) && !Is_null(i))
	    return ti->list_e();
    }
    return NULL; // block has no active operations
}

/* 
 * cfg_block::last_non_cti() -- returns pointer to the last
 * tnl element that is not a control instruction.  Routine returns NULL 
 * if the block is just a single control instruction. 
 */
tnle *
cfg_block::last_non_cti (void) const
{
    if (Is_cti(in_tail()->instr())) {
	if (in_tail() == in_head())
	    return NULL; // block is one cti instr
	else 
	    return in_tail()->list_e()->prev(); 
    } else {
	return in_tail()->list_e(); 
    }
}


tnle *
cfg_block::push (tnle *new_e)
{
    return this->insert_before(new_e, in_head()->list_e()); 
}

tnle *
cfg_block::pop ()
{
    return this->remove(in_head()->list_e()); 
}

tnle *
cfg_block::append (tnle *new_e)
{
    return this->insert_after(new_e, in_tail()->list_e()); 
}

tnle *
cfg_block::insert_before (tnle *new_e, tnle *pos)
{
    assert_msg(new_e->contents->is_instr(), 
	("cfg_block::insert_before() -- new_e not a TREE_INSTR")); 
    assert_msg(cfg_begin_node(new_e->contents) == NULL, 
	("cfg_block::insert_before() -- new_e has a cfg_node annotation")); 
    assert_msg(contains(pos), 
	("cfg_block::insert_before() -- pos does not belong to this block")); 
    annote *an = new annote(k_cfg_node, (void *)this);
    new_e->contents->annotes()->append(an);

    if (Is_cti(((tree_instr *)new_e->contents)->instr())) {
	set_in_cti((tree_instr *)(new_e->contents)); 
    }
    tree_instr *new_head = pos->contents == in_head() 
	? (tree_instr *)(new_e->contents) : in_head(); 
    tnle *ret = tnl->insert_before(new_e, pos); 
    set_in_head(new_head); 
    return ret; 
}

/* cfg_block::insert_after() -- insert the single instruction new_e
 * after the instruction pos in the tree_node_list. */
tnle *
cfg_block::insert_after (tnle *new_e, tnle *pos)
{
    assert_msg(new_e->contents->is_instr(), 
	("cfg_block::insert_after() -- new_e not a TREE_INSTR")); 
    assert_msg(cfg_begin_node(new_e->contents) == NULL, 
	("cfg_block::insert_after() -- new_e has a cfg_node annotation")); 
    assert_msg(contains(pos), 
	("cfg_block::insert_after() -- pos does not belong to this block")); 
    annote *an = new annote(k_cfg_node, (void *)this);
    new_e->contents->annotes()->append(an);

    if (Is_cti(((tree_instr *)new_e->contents)->instr())) {
	set_in_cti((tree_instr *)(new_e->contents)); 
    }
    tree_instr *new_tail = pos->contents == in_tail() 
	? (tree_instr *)(new_e->contents) : in_tail(); 
    tnle *ret = tnl->insert_after(new_e, pos); 
    set_in_tail(new_tail); 
    return ret; 
}

/* cfg_block::insert_after () -- insert the list of instructions mtnl
 * after the instruction pos in the tree_node_list. */
void
cfg_block::insert_after (tree_node_list *mtnl, tnle *pos)
{
    assert_msg(pos, ("cfg_block::insert_after() -- push not implemented"));
    while (!mtnl->is_empty()) 
	pos = insert_after(new tree_node_list_e(mtnl->pop()), pos); 
}

tnle *
cfg_block::remove (tnle *rem)
{
    assert_msg(rem->contents->is_instr(), 
	("cfg_block::remove() -- rem not a TREE_INSTR")); 
    assert_msg(this == cfg_begin_node(rem->contents), 
	("cfg_block::remove() -- instruction does not belong to this block")); 
    assert_msg(contains(rem), 
	("cfg_block::remove() -- instruction does not belong to this block")); 
    rem->contents->get_annote(k_cfg_node);

    if (in_head() == in_tail()) {
	assert(in_head() == rem->contents); 
	instruction *new_i;
	new_i = New_null();
	tree_instr *new_ti = new tree_instr(new_i); 
	new_ti->annotes()->append(new annote(k_cfg_node, (void *)this)); 
	tnl->insert_after(new_ti, in_head()->list_e()); 
	tnle *ret = tnl->remove(in_head()->list_e()); 
	set_in_head(new_ti); 
	set_in_tail(new_ti); 
	set_in_cti(NULL); 
	return ret; 
    } 

    tree_instr *new_head = rem->contents == in_head() 
	? (tree_instr *)(in_head()->list_e()->next()->contents)
	: in_head(); 
    tree_instr *new_tail = rem->contents == in_tail()
	? (tree_instr *)(in_tail()->list_e()->prev()->contents)
	: in_tail(); 
    if (rem->contents == in_cti()) {
	/* hope the user knows what he or she is doing when
	 * they remove the CTI instruction */
	set_in_cti(NULL); 
    }

    tnle *ret = tnl->remove(rem); 
    set_in_head(new_head); 
    set_in_tail(new_tail); 
    return ret; 
}


/* 
 * Verifies that cfg_block contains test. 测试该基本块是否包含某条指令.
 */
boolean 
cfg_block::contains(tnle *test)
{
    cfg_node_instr_iter cnii(this); 
    while (!cnii.is_empty()) {
	tree_instr *ti = cnii.step(); 
	if (test->contents == ti)
	    return TRUE; 
    }
    return FALSE; 
}


/**
 ** ------------- Print methods ------------- 
 **/

static void
print_node_list (FILE *fp, cfg_node_list *l)
{
    cfg_node_list_iter cnli(l);
    while (!cnli.is_empty()) {
	cfg_node *n = cnli.step();
	assert(n);  /* tjc */
        fprintf(fp, "%d", n->number());
        if (!cnli.is_empty())
            fprintf(fp, " ");
    }
}


void
cfg_node::print_base (FILE *fp)
{
    fprintf(fp, "p ");
    print_node_list(fp, preds());
    fprintf(fp, "\ts ");
    print_node_list(fp, succs());
    if (layout_succ())
	fprintf(fp, "\tl %d", layout_succ()->number());
}


void
cfg_begin::print (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: begin         ", number());
    print_base(fp);
    putc('\n', fp);
}


void
cfg_end::print (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: end           ", number());
    print_base(fp);
    putc('\n', fp);
}


void
cfg_label::print (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: label         ", number());
    print_base(fp);
    putc('\n', fp);
}


void
cfg_instr::print (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: instr         ", number());
    print_base(fp);
    putc('\n', fp);
}


void
cfg_block::print (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: block ", number());
    char *terms = "      ";
    if (ends_in_ubr())
	terms = "ubr   ";
    else if (ends_in_cbr())
	terms = "cbr   ";
    else if (ends_in_mbr())
	terms = "mbr   ";
    else if (ends_in_call())
	terms = "call  ";
    else if (ends_in_return())
	terms = "ret   ";
    else if (ends_in_cti())
	terms = "CTI?  ";
    char *shads = const_cast<char*>((in_shad()) ? "S " : "  ");
    fprintf(fp, "%s%s", terms, shads);

    print_base(fp);
    putc('\n', fp);
}


void
cfg_block::print_with_instrs (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: block ", number());
    char *terms = "      ";
    if (ends_in_ubr())
	terms = "ubr   ";
    else if (ends_in_cbr())
	terms = "cbr   ";
    else if (ends_in_mbr())
	terms = "mbr   ";
    else if (ends_in_call())
	terms = "call  ";
    else if (ends_in_return())
	terms = "ret   ";
    else if (ends_in_cti())
	terms = "CTI?  ";
    char *shads = const_cast<char*>((in_shad()) ? "S " : "  ");
    fprintf(fp, "%s%s", terms, shads);

    print_base(fp);
    putc('\n', fp);

    cfg_node_instr_iter cnii(this);
    while (!cnii.is_empty()) {
	instruction *in = cnii.step()->instr();
	if (in->architecture() == k_suif) 
	    in->print(fp, 6);	// indent 6
	else {
	    fputs("   ", fp);
	    ((machine_instr *)in)->print(fp);
	}
    }
    if (in_shad()) {
	instruction *in = in_shad()->instr();
	fprintf(fp, "     -- shadow: --\n");
	if (in->architecture() == k_suif) 
	    in->print(fp, 6);	// indent 6
	else {
	    fputs("   ", fp);
	    ((machine_instr *)in)->print(fp);
	}
    }
    
}


void
cfg_test::print (FILE *fp)
{
    fprintf(fp, "**** Block #%2d: test  ", number());
    print_base(fp);
    putc('\n', fp);
}


/**
 ** ------------- Protected functions ------------- 
 **/

void
cfg_node::add_succ (cfg_node *n)
{
    /* avoid duplicate edges */
    cfg_node_list_iter succ_iter(succs());
    while (!succ_iter.is_empty()) {
	cfg_node *succ = succ_iter.step();
	if (succ == n) return;
    }

    /* add the forward and backward edges */
    succs()->append(n);
    n->preds()->append(this);
}


void
cfg_node::remove_pred (cfg_node *n)
{
    remove_node(preds(), n);
    remove_node(n->succs(), this);
}


void
cfg_node::remove_succ (cfg_node *n)
{
    remove_node(succs(), n);
    remove_node(n->preds(), this);
}


/* 
 * remove_explicit_targets() -- This procedure leaves control
 * transfer instructions in place, but strips off their targets.
 * This is done when we clone basic blocks, so that users of
 * the library will be forced to explicitly set the targets
 * of the cloned block.  
 */
void
cfg_block::remove_explicit_targets ()
{
    /* Remove any explicit targets */
    instruction *lasti = ti2->instr(); 
    if (lasti->architecture() == k_suif) {
	unsigned nl, i;
	in_mbr *mbr2 = (in_mbr *)lasti; 

	switch (lasti->opcode()) {
	  case io_btrue: case io_bfalse: case io_jmp:
	    ((in_bj *)lasti)->set_target(NULL); 
	    break; 

	  case io_mbr:
	    mbr2->set_default_lab(NULL); 
	    nl = mbr2->num_labs(); 
	    for (i = 0; i < nl; ++i)
		mbr2->set_label(i, NULL); 
	    break; 

	  case io_cal: case io_ret:
	    break;

	  default:
	    break; 
	}

    } else if (Is_cti(lasti)) {
	if (Is_ubr(lasti) || Is_cbr(lasti)) {
	    ((mi_bj *)lasti)->set_target(NULL); 

	} else if (Is_mbr(lasti)) {
	    /* Do nothing.  This is handled in cfg::attach(). */

	} else if (Is_call(lasti) || Is_return(lasti)) {

	} else {
	    assert(0); 
	}
    }
}


cfg_block *
cfg_block::tnl_pred (void)
{
    tree_node_list_e *tnle; 
    cfg_node *nod; 
    if ((tnle = ti1->list_e()->prev())
	&& (nod = cfg_begin_node(tnle->contents))
	&& nod->is_block())
	return (cfg_block *)nod; 
    return NULL; 
}


cfg_block *
cfg_block::tnl_succ (void)
{
    tree_node_list_e *tnle; 
    cfg_node *nod; 
    tree_instr *ti = tiS ? tiS : ti2; 
    if ((tnle = ti->list_e()->next())
	&& (nod = cfg_begin_node(tnle->contents))
	&& nod->is_block())
	return (cfg_block *)nod; 
    return NULL; 
}


/**
 ** ------------- Static functions ------------- 
 **/

/* remove_node() -- remove all mentions of cfg_node n
 *		    from cfg_node_list l */
static void
remove_node (cfg_node_list *l, cfg_node *n)
{
    cfg_node_list_iter node_iter(l);
    boolean removed = FALSE;

    while (!node_iter.is_empty()) {
	cfg_node *node = node_iter.step();
	if (node == n) {
	    delete l->remove(node_iter.cur_elem());
	    removed = TRUE;
	}
    }
    assert_msg(removed, ("remove_node - node '%u' not found", n->number()));
}
