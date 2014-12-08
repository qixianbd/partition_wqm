/*  Control Flow Graph Implementation */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995,1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#define _MODULE_ "libcfg.a"

#pragma implementation "graph.h"

#include <suif1.h>
#include <machine.h>
#include "cfg.h"

/* local function prototypes */
static void add_cfg_edge(tree_node *t, void *x);
static void add_edge(tree_instr *ti, label_sym *target);


/*
 *  Build a new flow graph.  Flow graphs represent entire procedures.  
 */
cfg::cfg (tree_block *b, boolean build_blocks, 
    boolean break_at_call, boolean keep_l)
{
    /* initialization of private variables */
    tp = (tree_proc *)b; 
    nds = new cfg_node_array;
    en = NULL;
    ex = NULL;
    doms = NULL;
    pdoms = NULL;
    idom = NULL;
    ipdom = NULL;
    df = NULL;
    rdf = NULL;
    lp_depth = NULL;

    /* first pass -- create the nodes and fallthrough edges */
    graph_add_tree(b, NULL, build_blocks, break_at_call, keep_l); 

    set_entry_node(cfg_begin_node(b));
    set_exit_node(cfg_end_node(b));

    /* second pass -- set the successor(s) of the entry node */
    if (target_arch->family() != k_suif) {
	tree_node_list_iter tnli(b->body());
	while (!tnli.is_empty()) {
	    tree_node *tn = tnli.step(); 
	    if (!tn->is_instr())
		continue; 
	    tree_instr *ti = (tree_instr *)tn; 
	    if (ti->instr()->peek_annote(k_proc_entry) != NULL)
		entry_node()->add_succ(cfg_begin_node(ti)); 
	}
    }

    /* add an edge from the entry to the exit */
    entry_node()->add_succ(exit_node());

    /* third pass -- add the remaining edges.  postorder walk
     * because the tree_node being visited might be deleted */
    b->map(add_cfg_edge, (void *)exit_node(), FALSE);

    /* fourth pass -- set up layout information */
    if (keep_l) {
	for (unsigned i = 0; i < num_nodes(); ++i) {
	    cfg_node *nd = (*this)[i]; 
	    cfg_node *succ; 
	    tree_node_list_e *tnle; 
	    switch (nd->kind()) {
	    case CFG_BEGIN: 
		// grab non-end successor block
		succ = cfg_end_node(b->body()->head()->contents); 
		assert(nd->lsucc == NULL); 
		assert(succ->lpred == NULL); 
		nd->lsucc = succ; 
		succ->lpred = nd; 
		break; 

	    case CFG_END:
		// grab highest numbered successor block
		// DO NOT set pred's layout successor to END; breaks 
		// set_layout_succ()
		assert(nd->lpred == NULL); 
		break; 

	    case CFG_BLOCK:
		tnle = ((cfg_block *)nd)->in_tail()->list_e()->next(); 
		if (tnle == NULL) {
		    if (nd->succs()->count()) {
			succ = nd->succs()->head()->contents;
			if (succ->is_block())
			    ((cfg_block *)nd)->set_shadow(succ); 
		    }
		    break;
		}
		succ = cfg_end_node(tnle->contents); 
		assert(nd->lsucc == NULL); 
		assert(succ->lpred == NULL); 
		nd->lsucc = succ; 
		succ->lpred = nd; 
		break; 

	    case CFG_INSTR:
		tnle = ((cfg_instr *)nd)->instr()->list_e()->next(); 
		if (tnle == NULL)
		    break; 
		succ = cfg_end_node(tnle->contents); 
		assert(nd->lsucc == NULL); 
		assert(succ->lpred == NULL); 
		nd->lsucc = succ; 
		succ->lpred = nd; 
		break; 

	    case CFG_TEST:
	    case CFG_LABEL:
	    default:
		assert(0);
		break; 
	    }
	}

    } else {
	for (unsigned i = 0; i < num_nodes(); ++i) {
	    cfg_node *nd = (*this)[i]; 
	    if (!nd->is_block())
		continue; 
	    cfg_block *blk = (cfg_block *)nd; 

	    if (blk->succs()->count()
		&& (blk->succs()->head()->contents)->is_block()) {
		blk->set_shadow(blk->succs()->head()->contents); 
	    } 
	}
    }
}


/* 
 * Destroy the cfg graph and remove the cfg annotations on all of the
 * tree_instrs and label_syms.  Notice that this is not entirely
 * necessary since the cfg annotations will not be written out
 * with the procedure.
 */

static void remove_cfg_annotations(tree_block *tb);

cfg::~cfg ()
{
    for (unsigned i = 0; i < num_nodes(); i++) delete (*nds)[i];
    delete nds;
    if (doms) delete[] doms;
    if (pdoms) delete[] pdoms;
    if (idom) delete[] idom;
    if (ipdom) delete[] ipdom;
    if (df) delete[] df;
    if (rdf) delete[] rdf;
    if (lp_depth) delete[] lp_depth;

    remove_cfg_annotations(tproc());
}

void
remove_cfg_annotations(tree_block *tb)
{
    tree_node_list_iter tnli(tb->body());

    /* walk over the flat list of instructions looking for annotations */
    while (!tnli.is_empty()) {
	tree_instr *ti = (tree_instr *)tnli.step();
	ti->get_annote(k_cfg_node);
	if (Is_label(ti->instr()))
	    Get_label(ti->instr())->get_annote(k_cfg_node);
    }
    tb->get_annote(k_cfg_begin);
    tb->get_annote(k_cfg_end);
}


/* 
 * Return the number of nodes currently in the CFG.  
 */
unsigned
cfg::num_nodes ()
{
    int n = nds->ub();
    return (unsigned)n;
}


/* 
 * Return the number of edges or multiedges currently in the CFG. 
 */
unsigned 
cfg::num_edges() 
{
    unsigned ne = 0; 
    cfg_edge_iter edges(this, FALSE); 
    while (!edges.is_empty()) {
	edges.step(); 
	if (!edges.is_empty())
	    ++ne; 
    }
    return ne; 
}


unsigned 
cfg::num_multi_edges()
{
    unsigned ne = 0; 
    cfg_edge_iter edges(this, TRUE); 
    while (!edges.is_empty()) {
	edges.step();
	if (!edges.is_empty())
	    ++ne; 
    }
    return ne; 
}


/* 
 * Set the entry or exit node of the graph.  These should probably be
 * protected, because users don't get to add entry or end
 * nodes to a CFG.  
 */
void
cfg::set_entry_node (cfg_node *n)
{
    assert(n->is_begin());
    en = n;
}

void
cfg::set_exit_node (cfg_node *n)
{
    assert(n->is_end());
    ex = n;
}


/* attach() -- Link a free-floating cfg_block into an existing
 * cfg.  This is a helper function for clone_into() and new_empty_block(). 
 * nod is assumed to be a cfg_block.  nod's instructions are appended
 * to this' tree_node_list, and nod gets added to this CFG.  
 * All cfg edges and branch targets are cleared, except return blocks 
 * have the exit node set as a successor.  
 */
void 
cfg::attach(cfg_node *nod)
{
    assert(nod->kind() == CFG_BLOCK); 
    cfg_block *blk = (cfg_block *)nod; 
    tree_node_list *cfg_tnl = tproc()->body(); 

    // Make sure this is unattached
    assert(blk->parent() == NULL); 
    assert(blk->tnl != cfg_tnl); 
    assert((tree_instr *)blk->tnl->head()->contents == blk->in_head()); 
    assert((tree_instr *)blk->tnl->tail()->contents == blk->in_tail()); 
    cfg_node_list_iter cnlip(blk->preds()); 
    while (!cnlip.is_empty())
	assert(cnlip.step() == NULL); 
    cfg_node_list_iter cnlis(blk->preds()); 
    while (!cnlis.is_empty())
	assert(cnlis.step() == NULL); 
    assert(blk->layout_succ() == NULL); 
    assert(blk->tiS == NULL); 

    // Transfer instructions from blk->tnl to cfg_tnl
    // Destroy and change over blk->tnl
    blk->set_in_head(NULL); 
    while (!blk->tnl->is_empty()) {
	tree_node *tn = blk->tnl->pop(); 
	assert(tn->is_instr()); 
	if (blk->in_head() == NULL) // First iteration?
	    blk->set_in_head((tree_instr *)tn); 
	cfg_tnl->append(tn); 
    }
    blk->set_in_tail((tree_instr *)cfg_tnl->tail()->contents); 
    delete blk->tnl; 
    blk->tnl = cfg_tnl; 
    assert(blk->tnl->tail()->contents == blk->in_tail()); 

    blk->set_shadow(NULL); 
    blk->remove_explicit_targets(); 
    add_node(nod); 

    // If this is a return node, make the end node a successor
    if (blk->ends_in_return())
	blk->set_succ(0, exit_node()); 
}


/* 
 * clone_into() -- Make a copy of a block and link it into 
 * this cfg.  Return the clone to the user, so he can set its
 * successors and predecessors.  
 */
cfg_block *
cfg::clone_into(cfg_block *base)
{
    boolean same_scope = tproc() == base->parent()->tproc(); 
    base_symtab *stab_to_use = same_scope ? NULL : tproc()->proc_syms(); 
    cfg_block *clone = new cfg_block(base, stab_to_use); 
    attach(clone); 
    return clone; 
}


/* 
 * clone_into() -- Make a new empty block (actually a block with
 * just a label instruction for its contents) and link it into 
 * this cfg.  Return the empty block to the user, so he can set its
 * successors and predecessors.  
 */
cfg_block *
cfg::new_empty_block(void)
{
    cfg_block *empty = new cfg_block(tproc()->proc_syms()); 
    attach(empty); 
    return empty; 
}


/*
 *  Remove unreachable nodes from a flow graph.  Return true if any
 *  nodes are removed.  If boolean argument delete_instrs is true,
 *  invoke helper cfg_cleanup() before deleting each removed node.
 *
 *  This function simplifies finding dominators and may also be used
 *  after manually removing nodes (e.g. unwanted BEGIN and END markers,
 *  nop instructions, etc.) from the graph.
 *
 *  Note: There should never be unreachable nodes in the reverse graph.
 */

static void unreachable_visit(cfg_node *n, bit_set *mark);

boolean
cfg::remove_unreachable_blocks (boolean delete_instrs)
{
    boolean did_work = FALSE;
    unsigned num = num_nodes();
    unsigned i;

    /* find the reachable blocks */
    bit_set mark(0, num);
    unreachable_visit(entry_node(), &mark);

    /* copy the reachable nodes to a new array */
    cfg_node_array *new_nds = new cfg_node_array;
    for (i = 0; i < num; i++) {
	cfg_node *nd = node(i);
	if (mark.contains(i)) {
	    int new_num = new_nds->extend((void *)nd);
	    nd->set_number((unsigned)new_num);
	} else {
	    // Remove the unreachable node nd from the layout, closing the gap
	    // around it if possible.  It may be necessary to redirect the
	    // zero-th edge of nd's layout predecessor to flow to nd's layout
	    // successor.  This is because nd's layout pred might fall through
	    // to nd, which would inhibit using set_layout_succ() on the layout
	    // pred with an argument other than nd.  Only use this ploy when
	    // the layout pred is also a flow pred.  Thus it is unreachable and
	    // will be deleted later in the current loop; hence, the bogus
	    // control path will not be permanent.

	    cfg_node *olpred = nd->layout_pred();
	    cfg_node *olsucc = nd->layout_succ();
	    if (olsucc)
		nd->clear_layout_succ();
	    if (olpred) {
		olpred->clear_layout_succ();
		if (olsucc) {
		    if (occurs(olpred, nd->preds()))
			olpred->set_succ(0, olsucc);
		    olpred->set_layout_succ(olsucc);
		}
	    }

	    // Detach the unreachable node from its successors.

	    while (!nd->succs()->is_empty())
		nd->remove_succ((*nd->succs())[0]);

	    // Detach the unreachable node from its predecessors.  Necessary
	    // because predecessors remaining after the preceding loop will be
	    // visited after the current node has been deleted, at which point
	    // it would be invalid to use remove_succ() to break the flow link.

	    while (!nd->preds()->is_empty())
		nd->remove_pred((*nd->preds())[0]);

	    if (delete_instrs) cfg_cleanup(nd);
	    delete nd;

	    did_work = TRUE;
	}
    }

    delete nds;
    nds = new_nds;
    delete lp_depth;
    lp_depth = NULL;

    return did_work;
}

static void
unreachable_visit (cfg_node *n, bit_set *mark)
{
    /* mark the node as reachable */
    mark->add(n->number());

    /* visit its successors */
    cfg_node_list_iter succ_iter(n->succs());
    while (!succ_iter.is_empty()) {
	cfg_node *succ = succ_iter.step();
	if (!mark->contains(succ->number())) {
	    unreachable_visit(succ, mark);
	}
    }
}

static void discard_mbr_table(cfg_block *);	// used in next 2 methods

void
cfg::cfg_cleanup(cfg_node *nd)
{
    if (nd->is_block()) {
	cfg_block *blk = (cfg_block *)nd;

	if (blk->ends_in_mbr())
	    discard_mbr_table(blk);

	tree_instr *h = blk->in_head();
	tree_instr *t = blk->in_tail();
	while (h != t) {
	    tree_node *n = h->list_e()->next()->contents;
	    assert(n->is_instr());
	    delete blk->tnl->remove(h->list_e());
	    delete h;
	    h = (tree_instr *)n;
	}
	delete blk->tnl->remove(h->list_e());
	delete h;
	if (blk->tiS) {
	    delete blk->tnl->remove(blk->tiS->list_e());
	    delete blk->tiS;
	}
    }
}

/*
 * merge_block_sequences() -- For each sequence p_1,...,p_k of
 * control equivalent cfg_block's such that p_i immediately
 * precedes p_i+1, move the instructions of p_2,...,p_k
 * into p1 and transfer the successors of pk to p1, leaving
 * p_2,...,p_k isolated and content-free, except for labels.
 *
 * When excising a block that lies in the middle of a layout
 * chain, close the chain again by linking its layout prede-
 * cessor to its layout successor.
 *
 * Return TRUE iff any sequences are merged.
 *
 * If optional argument break_at_call is TRUE, break a sequence
 * at the first block that ends with a call instruction.
 *
 * Ignore sequences not reachable from the entry node (so that
 * the implementor doesn't need to worry about cycles).
 *
 * As a side effect, eliminate conditional or multiway branches
 * terminating blocks that have unique successors.
 */
boolean
cfg::merge_block_sequences(boolean break_at_call)
{
    boolean did_merge = FALSE;
    cfg_node_list *nodes = reverse_postorder_list();

    cfg_node_list_iter cnli(nodes);
    assert(!cnli.is_empty());

    do {
	cfg_node *pn = cnli.peek(); 
	cfg_node *sn = NULL;		// unique successor or else null

	cfg_node_list_iter snli(pn->succs());
	while (!snli.is_empty()) {
	    cfg_node *cn = snli.step();
	    if (!(sn = (cn == sn) ? sn : (sn ? NULL : cn)))
	        break;			// no unique successor
	}
	
	if (!sn || !pn->is_block()
	||  break_at_call && ((cfg_block*)pn)->ends_in_call()) {
	    pn = cnli.step();
	    continue;
	}

	cfg_block *pb = (cfg_block*)pn;

	// The current predecessor block (pb) has a unique successor (sn).
	// Save and clear pb's layout-successor link.  This make pb's contents
	// easier to edit by keeping the ubr to sn in the shadow slot.

	cfg_node *pb_los = pb->layout_succ();
	if (pb_los)
	    pb->clear_layout_succ();

	// If pb ends with a cbr or mbr, that instruction must not be
	// needed, since sn is pb's sole successor.  Take out the unneeded
	// CTI, even if the current merge doesn't succeed.  Replace the two
	// or more successor edges by a single edge.

	if (pb->ends_in_cbr() || pb->ends_in_mbr()) {
	    if (pb->ends_in_mbr())
		discard_mbr_table(pb);

	    // In flat low SUIF, pb's CTI can in general refer to earlier
	    // instructions in the block.  Excise and delete them all.

	    expunge_instr_tree(pb, pb->in_cti());

	    // Note that it is normal for a node with one control successor
	    // and no layout successor to have no CTI.  The shadow goto that
	    // makes the code correct in this case is established, if
	    // necessary, by the following set_succ() call.

	    pb->remove_succ(sn);		// remove all successor edges
	    pb->set_succ(0, sn);		// put one back
	}

	// If sn is not a block node, or if pb is not its only predecessor,
	// bail out of this merge, after reestablishing pb's layout link.

	if (!sn->is_block() || sn->preds()->count() != 1) {
	    pn = cnli.step();
	    if (pb_los)
		pb->set_layout_succ(pb_los);
	    continue;
	}

	// Block sb (a.k.a. node sn) will be emptied and disconnected.  Start
	// by dissolving its layout links, if any, but remember its layout
	// neighbors.
	
	cfg_block *sb = (cfg_block*)sn;
	cfg_node *sb_lop = sb->layout_pred();
	cfg_node *sb_los = sb->layout_succ();
	if (sb_lop)
	    sb_lop->clear_layout_succ();
	else if (pb_los == sb)
	    sb_lop = pb;
	if (sb_los)
	    sb->clear_layout_succ();

	pb->remove_succ(sb);

	// Transfer sb's non-label instructions to pb.  Recall that pb has no
	// exposed, block-terminating CTI at this point.  The instructions
	// appended go in front of the shadow goto.

	cfg_node_instr_iter sbi(sb);
	while (!sbi.is_empty()) {
	    tree_instr *ti = sbi.step();
	    if (!Is_label(ti->instr()))
		pb->append(sb->remove(ti->list_e()));
	}

	// Compensate for misfeature of cfg_block::append: it will have treated
	// a call instruction as a block-terminating CTI, even when the CFG has
	// not been with break_at_call == TRUE.

	if (!break_at_call && pb->ends_in_call())
	    pb->set_in_cti(NULL);

	// Transfer sb's successor edges to pb.  When j == 0, the set_succ() call
	// adjusts pb's shadow to reflect its new CTI.

	for (unsigned j = 0; !sb->succs()->is_empty(); j++) {
	    cfg_node *ss = sb->succs()->head()->contents;
	    sb->remove_succ(ss);
	    pb->set_succ(j, ss);
	}

	// If sb, the block just disconnected, had both a layout predecessor
	// and a layout successor, then put the latter right after the former
	// in the layout.  (Block pb might be either one of those, or neither
	// one.)

	if (sb_los && sb_lop)
	    sb_lop->set_layout_succ(sb_los);

	// If pb, the result of the current merge, originally had a layout
	// successor (pb_los) other than the now-disconnected block sb, try to
	// connect it after pb in the new layout.  This is always legal if
	// pb_los is a control successor of pb.  If not, it can be done unless
	// pb now ends with a cbr or a call, both of which require pb to fall
	// through to its layout successor.

        if (pb_los
	    && pb_los != sb
	    && (occurs(pb_los, pb->succs()) ||
		(!pb->ends_in_cbr() && !pb->ends_in_call())))
	    pb->set_layout_succ(pb_los);

	did_merge = TRUE;

    } while (!cnli.is_empty());

    return did_merge;
}


/*
 * discard_mbr_table(blk) -- Blk is known to end with an mbr that's
 * about to be rubbed out.  Find and neutralize its jump table
 * definition, so that the entries don't mention non-existent
 * labels in the .s file.  The table definition needs to remain,
 * because it might be used in setup computations that have been
 * scheduled out of the current block.  Delete all entries but one,
 * however, and replace the label in that one by zero.
 *
 * Each table entry is represented by a k_repeat_init annote on the
 * var_def of a compiler-generated static variable dedicated to
 * this mbr.  The annote data is a three-element immed list.  The
 * label symbol replaced by zero is the third element of the first
 * such triple.
 */
static void
discard_mbr_table(cfg_block *blk) {
    instruction *mbri = blk->in_cti()->instr(); 

    if (mbri->architecture() == k_suif)
	return;

    immed_list *il = (immed_list *)mbri->peek_annote(k_instr_mbr_tgts);

    var_sym *tab = (var_sym *)(*il)[1].symbol(); 
    assert(tab->is_var()); 
    var_def *def = tab->definition(); 

    immed_list *triple = (immed_list *) def->get_annote(k_repeat_init);
    assert(triple && triple->count() == 3);

    while ((il = (immed_list *)def->get_annote(k_repeat_init)))
	delete il;				// toss entries after 1st

    ((immed_list_e *)((*(dlist *)triple)[2]))->contents = immed(0);
    def->append_annote(k_repeat_init, triple);
}

/*
 * optimize_jumps() -- Replace jumps to jumps.  Return TRUE
 * if some jump is changed, else FALSE.
 *
 * Repeat the following passes until no change occurs:
 *
 * (1) Identify blocks that end with an unconditional jump,
 *     but are otherwise vacuous.  (Vacuous means: contains
 *     only labels, pseudo-ops, and null instructions.)
 * (2) Replace jumps to such blocks by jumps to their
 *     unique successors.
 */
boolean
cfg::optimize_jumps()
{
    boolean ever_changed, changed = FALSE;
    int nn = num_nodes();
    int i;
    cfg_node **redirect = new cfg_node *[nn];

    for (i = 0; i < nn; ++i) {
	cfg_node *scn = (*this)[i];
	tree_node_list_e *tnle;

	if ((scn->is_block())
	&&  (scn->succs()->count() == 1)
	&&  (!(tnle = ((cfg_block *)scn)->first_active_op())
	  || Is_ubr(((tree_instr *)tnle->contents)->instr())))
	    redirect[i] = scn->succs()->head()->contents;
	else
	    redirect[i] = NULL;
    }

    do {
	ever_changed = changed;
	changed = FALSE;

	for (i = 0; i < nn; ++i) {
	    cfg_node *scn = (*this)[i];
	    if (!scn->is_block())
		continue;

	    cfg_block *blk = (cfg_block *)scn;
	    for (int j = 0; j < blk->succs()->count(); ++j) {
		cfg_node *succ = (*blk->succs())[j];
		int succ_num = succ->number();
		cfg_node *new_succ = redirect[succ_num];

		/* Don't set changed unless a successor actually changes.
		 * Termination depends on real progress here. */

		if (!new_succ || new_succ == succ)
		    continue;

		/* If not a fall-through case, just use set_succ() to alter
		 * the branch target.  If succ is blk's (empty) fall-through
		 * successor, first clear any layout link between them. */

		cfg_node *fall_thru_los = NULL;

		if (j == 0 && !blk->ends_in_ubr() && !blk->ends_in_mbr())
		    fall_thru_los = blk->layout_succ();

		if (fall_thru_los)	// blk falls thru to empty layout succ
		    blk->clear_layout_succ();

		blk->set_succ(j, new_succ);

		/* If there was a layout chain from blk falling through succ to
		 * new_succ, close the chain after excising succ. */

		if (fall_thru_los && succ->layout_succ() == new_succ) {
		    succ->clear_layout_succ();
		    blk->set_layout_succ(new_succ);
		}

		/* Shorten any cycles of vacuous nodes by updating redirect[n]
		 * when the successor of vacuous node n is itself redirected. */

		if (redirect[blk->number()])
		    redirect[blk->number()] = new_succ;

		changed = TRUE;
	    }
	}
    } while (changed);

    return ever_changed;
}



/* 
 * remove_shadows() -- Walks through the blocks of the
 * procedure and removes shadow instructions if they jump to
 * the immediately following block, or if they can be absorbed
 * by inverting a conditional branch.
 *
 * This function should not be used until after all set_succ()
 * and set_layout_succ() calls have been made.  set_succ()
 * and set_layout_succ() expect shadow instructions to be 
 * in place; they may break if the shadows have been removed. 
 */
void 
cfg::remove_shadows()
{
    unsigned i; 
    for (i = 0; i < num_nodes(); ++i) {
	cfg_block *blk = (cfg_block *)((*this)[i]); 
	if (!blk->is_block() || blk->tiS == NULL)
	    continue; 
	tree_node_list_e *tnle = blk->tiS->list_e()->next(); 
	if (!tnle)
	    continue; 
	tree_node *tn = tnle->contents; 
	if (!tn->is_instr())
	    continue; 

	assert(blk->layout_succ() == NULL); 
	cfg_node *nod = cfg_begin_node(tn); 
	assert(nod); 
	if ((nod == (*blk->succs())[0])
	||  (blk->ends_in_cbr() && nod == (*blk->succs())[1]))
	    blk->set_layout_succ(nod);		/* may invert cbr */
    }
}


/* 
 * remove_layout_info() -- Clear all the layout links between
 * cfg_node's.  Uses clear_layout_succ(), so shadow instructions
 * may be inserted.  
 */
void 
cfg::remove_layout_info()
{
    unsigned i; 
    for (i = 0; i < num_nodes(); ++i) {
	cfg_node *scn = (*this)[i]; 
	scn->clear_layout_succ(); 
    }
    for (i = 0; i < num_nodes(); ++i) {
	cfg_node *scn = (*this)[i]; 
	assert(scn->layout_pred() == NULL); 
    }
}


/*
 *  Build a list of the flow graph nodes in reverse postorder.
 *  When forward == FALSE, build it in reverse, giving plain
 *  postorder.
 */

static void rpostord_visit(cfg_node_list *result, cfg_node *n, bit_set *mark,
			   boolean forward);

cfg_node_list *
cfg::reverse_postorder_list (boolean forward)
{
    cfg_node_list *result = new cfg_node_list;
    bit_set mark(0, num_nodes());
    rpostord_visit(result, entry_node(), &mark, forward);
    return result;
}


/*
 * The depth-first traversal visits a nodes successors in reverse order
 * simply to make the rpo more consistent with layout order.
 */

void
rpostord_visit (cfg_node_list *result, cfg_node *n, bit_set *mark,
		boolean forward)
{
    int i;
    for (i = (n->succs()->count() - 1); i >= 0; i--) {
	cfg_node *s = (*n->succs())[i];

	if (!s)
	    continue; 

	/* check if the node has already been visited */
	if (mark->contains(s->number())) continue;

	mark->add(s->number());
	rpostord_visit(result, s, mark, forward);
    }

    /* add this block to the front (for rpo) or back (plain po)
     * of the result list */
    if (forward)
	result->push(n);
    else
        result->append(n);
}


/*
 *  Find the dominators of each node.  The "remove_unreachable_blocks"
 *  method must be called prior to this or the results may be incorrect.
 */

void
cfg::find_dominators ()
{
    /* delete any old information */
    if (doms) delete[] doms;
    if (idom) delete[] idom;

    /* allocate and initialize the new bit vectors */
    doms = dominators(TRUE);
    idom = immed_dominators(TRUE);
}


/*
 *  Find the postdominators of each node.  A legal graph should never have
 *  unreachable nodes in the reverse graph.  If that is not true, this will
 *  not work correctly.
 */

void
cfg::find_postdominators ()
{
    /* delete any old information */
    if (pdoms) delete[] pdoms;
    if (ipdom) delete[] ipdom;

    /* compute the dominators */
    pdoms = dominators(FALSE);
    ipdom = immed_dominators(FALSE);
}


/*
 *  Compute (post)dominators.  This is a protected method that is used
 *  internally to do the actual work of finding dominators.
 */

bit_set *
cfg::dominators (boolean forward)
{
    cfg_node_list *node_list = reverse_postorder_list(forward);
    boolean changed = FALSE;
    unsigned num = num_nodes();

    /* allocate and initialize the new bit vectors */
    bit_set *d = new bit_set[num];
    for (unsigned i = 0; i < num; i++) {
	d[i].expand(0, num);
	d[i].universal();
    }

    /* set up the first node */
    cfg_node *start = (forward ? entry_node() : exit_node());
    d[start->number()].clear();
    d[start->number()].add(start->number());

    bit_set t(0, num_nodes());

    /* iterate until no changes */
    do {
	changed = FALSE;
	cfg_node_list_iter node_iter(node_list);
	while (!node_iter.is_empty()) {

	    cfg_node *n = node_iter.step();
	    if (n == start) continue;

	    /* get intersection of predecessors of n */
	    t.universal();
	    cfg_node_list_iter pred_iter(forward ? n->preds() : n->succs());
	    while (!pred_iter.is_empty()) {
		cfg_node *p = pred_iter.step();
		t *= d[p->number()];
	    }

	    /* include itself in dominator set */
	    t.add(n->number());

	    /* check if there were any changes */
	    if (d[n->number()] != t) {
		d[n->number()] = t;
		changed = TRUE;
	    }
	}
    } while (changed);

    delete node_list;
    return d;
}


/*
 *  Find the immediate (post)dominators.  This is a protected method that
 *  is only used internally to do the actual work of finding immediate
 *  dominators.
 */

cfg_node **
cfg::immed_dominators (boolean forward)
{
    unsigned num = num_nodes();
    unsigned root = (forward ? entry_node()->number() : exit_node()->number());
    bit_set *d = (forward ? doms : pdoms);

    cfg_node **im = new cfg_node*[num];
    im[root] = NULL;

    for (unsigned n = 0; n < num; n++) {

	/* the root node has no dominators */
	if (n == root) continue;

	/* remove self for comparison */
	d[n].remove(n);

	/* check nodes in dominator set */
	boolean found = FALSE;
	bit_set_iter diter(&d[n]);
	while (!diter.is_empty()) {
	    unsigned i = diter.step();

	    if (i < num && d[n] == d[i]) {
		found = TRUE;
		im[n] = node(i);
	    }
	}

	if (!found) {
	    warning_line(NULL, "can't find idom of node %u", n);
	    im[n] = NULL;
	}

	/* re-add self when done */
	d[n].add(n);
    }

    return im;
}


/*
 *  Find the dominance frontier of each node.  The dominators must have
 *  already been computed.
 */

void
cfg::find_df ()
{
    assert_msg(doms, ("cfg::find_df - need to compute dominators first"));

    /* delete any old information */
    if (df) delete[] df;

    df = new bit_set[num_nodes()];
    dom_frontiers(entry_node(), TRUE);
}


/*
 *  Find the reverse dominance frontier of each node.  The postdominators
 *  must have already been computed.
 */

void
cfg::find_reverse_df ()
{
    assert_msg(pdoms, ("cfg::find_reverse_df - need postdominators first"));

    /* delete any old information */
    if (rdf) delete[] rdf;

    rdf = new bit_set[num_nodes()];
    dom_frontiers(exit_node(), FALSE);
}


/*
 *  Compute dominance frontiers using a postorder traversal of the dominator
 *  tree.  This is a protected method that is only used internally.
 */

void
cfg::dom_frontiers (cfg_node *x, boolean forward)
{
    unsigned n, num = num_nodes();
    cfg_node **xidom;
    bit_set *dom_front;
    cfg_node_list *nodes;

    if (forward) {
	xidom = idom;
	dom_front = df;
	nodes = x->succs();
    } else {
	xidom = ipdom;
	dom_front = rdf;
	nodes = x->preds();
    }

    /* visit all children (i.e. immediate dominatees) first */
    for (n = 0; n < num; n++) {
	if (xidom[n] == x) {
	    dom_frontiers(node(n), forward);
	}
    }

    /* calculate dominance frontier, from paper RCytron_89 */
    dom_front[x->number()].expand(0, num);

    /* local loop, uses CFG */
    cfg_node_list_iter succ_iter(nodes);
    while (!succ_iter.is_empty()) {
	cfg_node *s = succ_iter.step();

	if (x != xidom[s->number()]) {
	    dom_front[x->number()].add(s->number());
	}
    }

    /* up loop, uses dominator tree */
    for (n = 0; n < num; n++) {
	if ((xidom[n] == x) && !dom_front[n].is_empty()) {
	    for (unsigned y = 0; y < num; y++) {
		if (dom_front[n].contains(y) && (x != xidom[y])) {
		    dom_front[x->number()].add(y);
		}
	    }
	}
    }
}


/* Access routines for dominators and dominance frontiers. */

boolean
cfg::dominates (int n_dominator, int n_dominatee)
{
    assert_msg(doms, ("cfg::dominates() -- run find_dominators() first"));
    return doms[n_dominatee].contains(n_dominator);
}

boolean
cfg::dominates (cfg_node *dominator, cfg_node *dominatee)
{
    return dominates(dominator->number(), dominatee->number());
}

boolean
cfg::postdominates (int n_dominator, int n_dominatee)
{
    assert_msg(pdoms, ("cfg::postdominates() -- "
		       "run find_postdominators() first"));
    return pdoms[n_dominatee].contains(n_dominator);
}

boolean
cfg::postdominates (cfg_node *dominator, cfg_node *dominatee)
{
    return postdominates(dominator->number(), dominatee->number());
}

cfg_node *
cfg::immed_dom (int n)
{
    assert_msg(idom, ("cfg::immed_dom() -- run find_dominators() first"));
    return idom[n];
}

cfg_node *
cfg::immed_dom (cfg_node *n)
{
    return immed_dom(n->number());
}

cfg_node *
cfg::immed_postdom (int n)
{
    assert_msg(ipdom, ("cfg::immed_postdom() -- "
		       "run find_postdominators() first"));
    return ipdom[n];
}

cfg_node *
cfg::immed_postdom (cfg_node *n)
{
    return immed_postdom(n->number());
}

bit_set *
cfg::dom_frontier (int n)
{
    assert_msg(df, ("cfg::dom_frontier() -- run find_df first"));
    return &df[n];
}

bit_set *
cfg::dom_frontier (cfg_node *n)
{
    return dom_frontier(n->number());
}

bit_set *
cfg::reverse_dom_frontier (int n)
{
    assert_msg(rdf, ("cfg::reverse_dom_frontier() -- "
		     "run find_reverse_df() first"));
    return &rdf[n];
}

bit_set *
cfg::reverse_dom_frontier (cfg_node *n)
{
    return reverse_dom_frontier(n->number());
}


void 
cfg::find_natural_loops()
{
    int i;
    int num = num_nodes();
    int *node_stack = new int[num]; 

    if (lp_depth)
	delete[] lp_depth; 
    lp_depth = new int[num]; 

    assert(doms);

    /* initialize */
    for (i = 0; i < num; i++) 
	lp_depth[i] = 0; 

    /* find all the loop entries and loop-back blocks */
    for (i = 0; i < num; i++) {
	cfg_node *cn = node(i);
	if (cn->is_block()) {
	    int top_stack = 0; 
	    bit_set this_loop(0, num); 

	    /* if some predecessor induces a back edge, 
	     * then use it to seed the loop
	     */
	    cfg_node_list_iter pred_iter(cn->preds());
	    while(!pred_iter.is_empty()) {
		cfg_node *pred_cn = pred_iter.step();
		if (dominates(cn, pred_cn)) 
		    node_stack[top_stack++] = pred_cn->number(); 
	    }

	    /* there was some back edge, so this is a header */
	    if (top_stack) {
		++lp_depth[i]; 
		this_loop.add(i); 
	    }

	    /* walk backwards around loop, adding nodes */
	    while (top_stack) {
		int top = node_stack[--top_stack]; 
		if (!this_loop.contains(top)) {
		    this_loop.add(top); 
		    ++lp_depth[top]; 
		    cfg_node_list_iter top_iter(node(top)->preds());
		    while (!top_iter.is_empty()) {
			int m = top_iter.step()->number(); 
			node_stack[top_stack++] = m; 
			assert(top_stack < num); 
		    }
		}
	    }
	}
    }

    delete[] node_stack; 
}

int
cfg::loop_depth (int n)
{
    assert_msg(lp_depth, ("cfg::loop_depth() -- "
	"run find_natural_loops() first"));
    return lp_depth[n];
}

int
cfg::loop_depth (cfg_node *n)
{
    return loop_depth(n->number());
}

void 
cfg::set_loop_depth(int n, int d)
{
    assert_msg(lp_depth, ("cfg::loop_depth() -- "
	"run find_natural_loops() first"));
    lp_depth[n] = d;
}

void 
cfg::set_loop_depth(cfg_node *n, int d)
{
    set_loop_depth(n->number(), d);
}


boolean
cfg::is_loop_begin (int n)
{
    return is_loop_begin(node(n));
}

boolean
cfg::is_loop_begin (cfg_node *cn)
{
    cfg_node_list_iter pred_iter(cn->preds());
    while (!pred_iter.is_empty()) {
	cfg_node *pred_cn = pred_iter.step();
	if (dominates(cn, pred_cn)) return TRUE;
    }
    return FALSE;
}

boolean
cfg::is_loop_end (int n)
{
    return is_loop_end(node(n));
}

boolean
cfg::is_loop_end (cfg_node *cn)
{
    cfg_node_list_iter succ_iter(cn->succs());
    while (!succ_iter.is_empty()) {
	cfg_node *succ_cn = succ_iter.step();
	if (dominates(succ_cn, cn)) return TRUE;
    }
    return FALSE;
}

boolean
cfg::is_loop_exit (int n)
{
    return is_loop_exit(node(n));
}

boolean
cfg::is_loop_exit (cfg_node *cn)
{
    cfg_node_list_iter succ_iter(cn->succs());
    while (!succ_iter.is_empty()) {
	cfg_node *succ_cn = succ_iter.step();
	if (loop_depth(succ_cn) != loop_depth(cn)) return TRUE;
    }
    return FALSE;
}


/**
 ** ------------- Protected functions ------------- 
 **/

/* 
 * add_node() -- add a node to a cfg, without any error checking.  
 */
void
cfg::add_node (cfg_node *n)
{
    int num = nds->extend((void *)n);
    n->set_number((unsigned)num);
    n->set_parent(this);
    lp_depth = (int *)realloc(lp_depth, (num + 1) * sizeof(int)); 
    assert_msg(lp_depth != NULL, ("cfg::add_node -- "
	"unable to reallocate lp_depth array")); 
}


/* 
 * graph_add_list() -- cfg constructor helper for handling
 * tree_node_lists.  Has separate cases for maximal basic
 * block and single-instruction basic block cases.  Mutually
 * recursive with graph_add_tree() to support high-SUIF
 * structures.  
 */
cfg_node *
cfg::graph_add_list (tree_node_list *list, cfg_node *prev, 
    boolean block, boolean break_at_call, boolean keep_l)
{
    annote *an;

    tree_node_list_iter iter(list);
    while (!iter.is_empty()) {
	tree_node *tn = iter.step();

	/* check for and deal with non-instruction AST nodes */
	if (!tn->is_instr()) {
	    prev = graph_add_tree(tn, prev, block, break_at_call, keep_l); 
	    continue;
	}

	/* the rest of this code deals with instructions: */

	tree_instr *ti = (tree_instr *)tn;
	tree_instr *real_ti = ti;
	tree_instr *tiFirst = ti; 
	instruction *i = ti->instr();

	/* first handle the easy case of nodes for individual instructions */

	if (!block) {

	    /* create a new node for the instruction */
	    cfg_instr *inode = new cfg_instr(ti);
	    an = new annote(k_cfg_node, (void *)inode);
	    ti->annotes()->append(an);

	    add_node(inode);
	    if (prev) 
		prev->add_succ(inode);

	    /* record the positions of labels */
	    if (Is_label(i)) {
		label_sym *lab = Get_label(i);
		an = new annote(k_cfg_node, (void *)inode);
		lab->annotes()->append(an);
	    }

	    /* check if the instruction falls through to its successor. */
	    if (Is_cti(i)) {
	    	if (Is_cbr(i) || Is_call(i))
		    prev = inode;
		else if (Is_ubr(i) || Is_mbr(i) || Is_return(i))
		    prev = NULL;
		else
		    assert(0); 
	    } else {
		prev = inode; 
	    }

	    continue;
	}
	
	/* now comes the harder task of building basic blocks.... */

	boolean end_block = FALSE;
	cfg_block *blknode = new cfg_block(list, ti, ti);

	add_node(blknode);
	if (prev) 
	    prev->add_succ(blknode);
	prev = blknode;

	/* check for label instructions */
	while (Is_label(i)) {

	    /* record the label position */
	    label_sym *lab = Get_label(i);
	    an = new annote(k_cfg_node, (void *)blknode);
	    lab->annotes()->append(an);

	    /* record the node for the instruction */
	    an = new annote(k_cfg_node, (void *)blknode);
	    ti->annotes()->append(an);

	    /* read the next instruction */
	    if (iter.is_empty() || !iter.peek()->is_instr()) {
		end_block = TRUE;
		break;
	    }
	    real_ti = ti = (tree_instr *)iter.step();
	    blknode->extend(ti);
	    assert(ti->is_instr());
	    i = ti->instr();
	}

	/* if there's nothing but labels then this is the end */
	if (end_block) continue;

	while (TRUE) {

	    /* record the node for the instruction */
	    an = new annote(k_cfg_node, (void *)blknode);
	    ti->annotes()->append(an);

	    /* check for branch instruction that don't fall through. */
	    if (Is_cti(i) && !Is_cbr(i) && !Is_call(i)) {
		prev = NULL;
		break;
	    }

	    /* check for other branch instructions */
	    if (Is_cbr(i))
		break;

	    /* break basic blocks at calls if flag is set */
	    if (Is_call(i) && break_at_call)
		break; 

	    /* check if there is another non-label instruction after this */
	    if (iter.is_empty() ||
		!(ti = (tree_instr *)iter.peek())->is_instr() ||
		(Is_label(ti->instr())))
		break;

	    /* read the next instruction */
	    real_ti = ti = (tree_instr *)iter.step();
	    blknode->extend(ti);
	    assert(ti->is_instr());
	    i = ti->instr();
	}

	/* Handle shadow instructions at the end of blocks */
	if (Is_ubr(i)) {
	    if (tiFirst == ti) {
		/* Insert label to make up body of block */
		tree_proc *tp = tproc();
		proc_symtab *par_symtab = tp->proc_syms(); 
		label_sym *new_lab = par_symtab->new_unique_label(); 
		new_lab->annotes()->append(new annote(k_cfg_node,
		    (void *)blknode));
		instruction *new_i = New_lab(new_lab);
		tree_instr *new_ti = new tree_instr(new_i); 
		new_ti->annotes()->append(new annote(k_cfg_node, 
		    (void *)blknode)); 
		list->insert_before(new_ti, ti->list_e()); 
		blknode->set_in_head(new_ti); 
	    } 
	    if (keep_l) {
		blknode->set_in_cti(ti); 
	    } else {
		blknode->set_in_tail((tree_instr *)ti->list_e()
		    ->prev()->contents); 
		blknode->set_in_cti(NULL); 
		blknode->tiS = ti; 
	    }
	} else if (Is_cbr(i) || (Is_call(i) && break_at_call)) {
	    blknode->set_in_cti(real_ti);    /* fixed bug here -tjc */
	    /* check for shadow goto */
	    tree_instr *tiPossible; 
	    if (!keep_l
	     && !iter.is_empty() 
	     && (tiPossible = (tree_instr *)iter.peek())->is_instr() 
	     && Is_ubr(i = tiPossible->instr())) {
		iter.step(); 
		an = new annote(k_cfg_node, (void *)blknode);
		tiPossible->annotes()->append(an);
		blknode->tiS = tiPossible; 
		prev = NULL;			// no fall thru here
	    }
	} else if (Is_mbr(i) || Is_return(i)) {
	    blknode->set_in_cti(ti); 
	}

	assert_msg((!blknode->in_cti()) || Is_cti(blknode->in_cti()->instr()),
		   ("CTI isn't really a CTI!\n"));
    }

    return prev;
}


/* 
 * graph_add_tree() -- cfg constructor helper for handling
 * tree_nodes.  Separate cases handle different high-SUIF
 * constructs.  Mutually recursive with graph_add_list()
 * to handle the tree_node_list's embedded in high-SUIF
 * constructs.  
 */
cfg_node *
cfg::graph_add_tree (tree_node *t, cfg_node *prev, 
    boolean block, boolean break_at_call, boolean keep_l)
{
    cfg_node *node;
    cfg_node *save = NULL;
    cfg_node *save_contlab = NULL;
    cfg_node *save_toplab = NULL;
    cfg_node *save_jumpto = NULL;
    annote *an;

    /* add the BEGIN marker node */
    node = new cfg_begin(t);
    an = new annote(k_cfg_begin, (void *)node);
    t->annotes()->append(an);
    add_node(node);
    if (prev) prev->add_succ(node);
    prev = (target_arch->family() == k_suif) ? node : NULL;

    /* add the END marker node */
    cfg_node *end = new cfg_end(t);
    an = new annote(k_cfg_end, (void *)end);
    t->annotes()->append(an);
    add_node(end);

    switch (t->kind()) {

	case TREE_BLOCK: {
	    tree_block *tb = (tree_block *)t;
	    prev = graph_add_list(tb->body(), prev, 
		block, break_at_call, keep_l);
	    break;
	}

	case TREE_FOR: {
	    tree_for *tf = (tree_for *)t;
	    prev = graph_add_list(tf->lb_list(), prev, 
		block, break_at_call, keep_l); 
	    prev = graph_add_list(tf->ub_list(), prev, 
		block, break_at_call, keep_l);
	    save = graph_add_list(tf->step_list(), prev, 
		block, break_at_call, keep_l);
	    prev = graph_add_list(tf->landing_pad(), save, 
		block, break_at_call, keep_l);

	    /* create a separate node for the non-existent "top" label */
	    if (!block) {
		node = new cfg_label(t);
		an = new annote(k_cfg_toplab, (void *)node);
		t->annotes()->append(an);
		add_node(node);
		if (prev) prev->add_succ(node);
		prev = node;
		save_toplab = node;
	    }

	    prev = graph_add_list(tf->body(), prev, 
		block, break_at_call, keep_l);

	    /* create a separate node for the "continue" label */
	    if (!block) {
		node = new cfg_label(t);
		add_node(node);
		if (prev) prev->add_succ(node);
		prev = node;
		save_contlab = node;
	    }

	    /* create a new node for the "test" part */
	    node = new cfg_test(tf);
	    an = new annote(k_cfg_test, (void *)node);
	    t->annotes()->append(an);
	    add_node(node);
	    if (prev) prev->add_succ(node);
	    prev = node;

	    break;
	}

	case TREE_LOOP: {
	    tree_loop *tl = (tree_loop *)t;

	    /* create a separate node for the "top" label */
	    if (!block) {
		node = new cfg_label(t);
		add_node(node);
		if (prev) prev->add_succ(node);
		prev = node;
		save_toplab = node;
	    }

	    prev = graph_add_list(tl->body(), prev, 
		block, break_at_call, keep_l);

	    /* create a separate node for the "continue" label */
	    if (!block) {
		node = new cfg_label(t);
		add_node(node);
		if (prev) prev->add_succ(node);
		prev = node;
		save_contlab = node;
	    }

	    prev = graph_add_list(tl->test(), prev, 
		block, break_at_call, keep_l);
	    break;
	}

	case TREE_IF: {
	    tree_if *ti = (tree_if *)t;
	    prev = graph_add_list(ti->header(), prev, 
		block, break_at_call, keep_l);
	    save = graph_add_list(ti->then_part(), prev, 
		block, break_at_call, keep_l);
	    prev = NULL;

	    /* create a separate node for the "jumpto" label */
	    if (!block && !ti->else_part()->is_empty()) {
		node = new cfg_label(t);
		add_node(node);
		prev = node;
		save_jumpto = node;
	    }

	    prev = graph_add_list(ti->else_part(), prev, 
		block, break_at_call, keep_l);
	    break;
	}

	default: {
	    assert(FALSE);
	}
    }

    // PROBLEM: don't want non-return instructions that are
    // garbage at the end of the procedure to show end as a
    // successor, because they don't actually return.  But we
    // need the next line for TREE_FOR's, etc. where flow
    // falls out of the end.  Commented out for now.  
    // if (prev) prev->add_succ(end);

    switch (t->kind()) {

	case TREE_FOR: {
	    tree_for *tf = (tree_for *)t;

	    /* mark the position of the "continue" label */
	    if (block) save_contlab = cfg_test_node(tf);
	    an = new annote(k_cfg_node, (void *)save_contlab);
	    tf->contlab()->annotes()->append(an);

	    /* the "break" label is at the END node */
	    an = new annote(k_cfg_node, (void *)node);
	    tf->brklab()->annotes()->append(an);

	    /* add an edge from before the landing pad to the END */
	    assert(save);
	    save->add_succ(node);

	    /* add an edge from the test node to the top of the loop */
	    if (block) {
		save_toplab = save_contlab;
		if (!tf->body()->is_empty()) {
		    save_toplab = cfg_begin_node(tf->body()->head()->contents);
		}
	    }
	    cfg_test_node(tf)->add_succ(save_toplab);

	    break;
	}

	case TREE_LOOP: {
	    tree_loop *tl = (tree_loop *)t;

	    /* find the position of the "continue" label */
	    if (block) {
		save_contlab = node;
		if (!tl->test()->is_empty()) {
		    save_contlab =cfg_begin_node(tl->test()->head()->contents);
		}
	    }
	    an = new annote(k_cfg_node, (void *)save_contlab);
	    tl->contlab()->annotes()->append(an);

	    /* find the position of the "top" label */
	    if (block) {
		save_toplab = save_contlab;
		if (!tl->body()->is_empty()) {
		    save_toplab = cfg_begin_node(tl->body()->head()->contents);
		}
	    }
	    an = new annote(k_cfg_node, (void *)save_toplab);
	    tl->toplab()->annotes()->append(an);

	    /* the "break" label is at the END node */
	    an = new annote(k_cfg_node, (void *)node);
	    tl->brklab()->annotes()->append(an);

	    break;
	}

	case TREE_IF: {
	    tree_if *ti = (tree_if *)t;

	    /* the "jumpto" label is at the beginning of the ELSE part */
	    if (ti->else_part()->is_empty()) {
		save_jumpto = node;
	    } else if (block) {
		save_jumpto =cfg_begin_node(ti->else_part()->head()->contents);
	    }
	    an = new annote(k_cfg_node, (void *)save_jumpto);
	    ti->jumpto()->annotes()->append(an);

	    /* the THEN part skips over the ELSE part */
	    if (save) save->add_succ(node);

	    break;
	}

	default: {
	    break;
	}
    }

    return node;
}


/**
 ** ------------- Local functions ------------- 
 **/

/*
 *  This function is mapped across the AST to add the branch edges after
 *  the flow graph nodes have been created.  A pointer to the exit node
 *  should be passed in the "x" parameter.
 * 
 *  This function doesn't add fallthrough edges; that's done in 
 *  graph_add_list(). 
 * 
 *  This function should not call any of the set_succ() functions, 
 *  because it has to work for both block and instr mode, and there
 *  is no current way to set_succ() on a cfg_instr object.  
 *  Unlike set_succ(), we don't have to bother to remove old
 *  predecessor pointers.  We're setting them up for the first time.  
 */

static void
add_cfg_edge (tree_node *t, void *x)
{
    if (t->is_instr()) {
	tree_instr *ti = (tree_instr *)t;
	instruction *i = ti->instr();
	machine_instr *mi = (machine_instr *)i;
	cfg_node *this_node = cfg_end_node(ti);
	assert_msg(this_node, ("add_cfg_edge - cannot find node for "
	    "instruction (%d)", ti->instr()->number()));

	// Multiway branches
	if (Is_mbr(i)) {
	    assert_msg(this_node->succs()->count() == 0,
		("add_cfg_edge - mbr already has some successors"));
	    if (i->architecture() == k_suif) {
		in_mbr *mbri = (in_mbr *)i;
		for (unsigned n = 0; n < mbri->num_labs(); n++)
		    add_edge(ti, mbri->label(n));
		add_edge(ti, mbri->default_lab());  /* I swapped this - tjc */
	    } else {
		immed_list *iml = (immed_list *)
		    mi->peek_annote(k_instr_mbr_tgts);
		assert_msg(iml, ("add_cfg_edge - no mbr targets"));
		int num = (*iml)[0].integer();
		sym_node *dtable_sym = (*iml)[1].symbol();
		assert(dtable_sym->is_var());
		var_sym *dtable_var = (var_sym *)dtable_sym;
		var_def *dtable_def = dtable_var->definition();

		// Remove the table definitions and set up dummy successors
		immed_list **tgts = new immed_list*[num];
		int n;
		for (n = 0; n < num; n++) {
		    tgts[n] = (immed_list *)dtable_def
			->get_annote(k_repeat_init);
		    assert(tgts[n]);
		    immed im = (*tgts[n])[2];
		    label_sym *lsym = (label_sym *)im.symbol();
		    cfg_node *targ = lsym ? label_cfg_node(lsym) : NULL;
		    this_node->succs()->append(new cfg_node_list_e(targ));

		    if (targ && !occurs(this_node, targ->preds()))
			targ->preds()->append(this_node);
		}

		// Put the table definitions back
		for (n = 0; n < num; n++)
		    dtable_def->append_annote(k_repeat_init, tgts[n]);

		delete tgts;
	    }
            return;    /* added: tjc */
	}

	// Conditional branches
	//   Allow for the possibility that a cbr node has no successors
	//   because an immediately following ubr has been absorbed as a
	//   shadow instruction.
	if (Is_cbr(i)) {
	    instruction *cti[2] = { NULL, i };	// possible shadow, then cbr
	    if (this_node->succs()->count() == 0) {
		assert_msg(this_node->is_block(),
			   ("add_cfg_edge - missing cbr fall thru successor"));
		tree_instr *tiS = ((cfg_block *)this_node)->in_shad();
		assert_msg(tiS, ("add_cfg_edge - no successors, no shadow"));
		cti[0] = tiS->instr();
	    }
	    for (int j = 0; j < 2; j++) {
		if (!cti[j])
		    continue;
		label_sym *target = (label_sym *)Get_target(cti[j]);
		cfg_node *target_node = label_cfg_node(target);
		assert_msg(target_node, ("add_cfg_edge - cannot find node for "
					 "label `%s'", target->name()));
		this_node->succs()->append(target_node);
		if (!occurs(this_node, target_node->preds()))
		    target_node->preds()->append(this_node);
	    }
	    assert_msg(this_node->succs()->count() == 2,
		       ("add_cfg_edge - cbr has wrong number of successors"));
            return;    /* added: tjc */
	}

	// Unconditional branches
	if (Is_ubr(i)) {
	    label_sym *target = (label_sym *)Get_target(i);
	    cfg_node *target_node = label_cfg_node(target);
	    assert_msg(target_node, ("add_cfg_edge - cannot find node for "
		"label `%s'", target->name()));

	    if (this_node->is_block()) {
		cfg_block *blk = (cfg_block *)this_node;
		if (blk->in_shad() == ti) {
		    this_node->set_fall_succ(target_node);
		} else {
		    assert_msg(this_node->succs()->count() == 0,
			("add_cfg_edge - expected no ubr successors"));
		    this_node->succs()->push(target_node);
		    if (!occurs(this_node, target_node->preds()))
			target_node->preds()->push(this_node);
		}
	    } else if (this_node->is_instr()) {
		cfg_instr *ins = (cfg_instr *)this_node;
		assert_msg(ins->succs()->count() == 0,
		    ("add_cfg_edge - expected no ubr successors (instr)"));
		ins->succs()->push(target_node);
		if (!occurs(ins, target_node->preds()))
		    target_node->preds()->push(this_node);
	    } else {
		assert_msg(FALSE, ("add_cfg_edge - unknown node type"));
	    }
            return;    /* added: tjc */
	}

	// Link return to exit node
	if (Is_return(i)) {
	    assert_msg(this_node->succs()->count() == 0,
		("add_cfg_edge - expected no return successors"));
	    // add an edge to the exit node
	    this_node->succs()->append((cfg_node *)x);
	    if (!occurs(this_node, ((cfg_node *)x)->preds()))
		((cfg_node *)x)->preds()->append(this_node);
            return;    /* added: tjc */
	}
    }
}


/* 
 * add_edge() -- This function should be removed when we do
 * the low-SUIF implementation.  It currently exists to handle
 * low-SUIF mbr's, and hasn't been tested.  We need to come up with
 * a reasonable way to number the default label in low-SUIF mbr's.  
 *
 * Ok, low-SUIF mbr's work now.  It's been tested more with cfg_blocks,
 * but cfg_instrs seem to work too.
 * The mbr's default label is numbered last and thus must be added last. -- tjc
 */
static void
add_edge (tree_instr *ti, label_sym *target)
{
    cfg_node *target_node = label_cfg_node(target);
    assert_msg(target_node, ("graph_update_tree - cannot find node for "
	"label `%s'", target->name()));

    cfg_node *this_node = cfg_end_node(ti);
    assert_msg(this_node, ("graph_update_tree - cannot find node for "
	"instruction (%d)", ti->instr()->number()));

    // This didn't work for several reasons...
    // this_node->set_succ(this_node->succs()->count(), target_node);

    // This seems to work in both block and instruction modes
    this_node->succs()->append(target_node); 
    if (!occurs(this_node, target_node->preds()))
	target_node->preds()->append(this_node); 
}



void
cfg::print (FILE *fp, boolean show_layout)
{
    fprintf(fp, "**** CFG contains %d blocks\n", num_nodes()); 

    if (show_layout) {
	bit_set done(0, num_nodes()); 
	for (unsigned i = 0; i < num_nodes(); i++) {
	    if (done.contains(i))
		continue;
	    cfg_node *curr = node(i); 
	    while (curr->layout_pred())
		curr = curr->layout_pred(); 
	    while (curr) {
		curr->print(fp); 
		done.add(curr->number()); 
		curr = curr->layout_succ(); 
	    }
	}
    } else {
	for (unsigned i = 0; i < num_nodes(); ++i) 
	    node(i)->print(fp); 
    }
}


void
cfg::print_with_instrs (FILE *fp, boolean show_layout)
{
    fprintf(fp, "**** CFG contains %d blocks\n", num_nodes()); 

    if (show_layout) {
	bit_set done(0, num_nodes()); 
	for (unsigned i = 0; i < num_nodes(); i++) {
	    if (done.contains(i))
		continue;
	    cfg_node *curr = node(i); 
	    while (curr->layout_pred())
		curr = curr->layout_pred(); 
	    while (curr) {
		if (curr->is_block())
		    ((cfg_block *)curr)->print_with_instrs(fp); 
		else
		    curr->print(fp); 
		done.add(curr->number()); 
		curr = curr->layout_succ(); 
	    }
	}
    } else {
	for (unsigned i = 0; i < num_nodes(); ++i)  {
	    if (node(i)->is_block())
		((cfg_block *)node(i))->print_with_instrs(fp); 
	    else
		node(i)->print(fp); 
	}
    }
}


cfg_edge_iter::cfg_edge_iter (cfg *base, boolean multigraph) 
{
    this->base = base; 
    this->multigraph = multigraph; 
    reset(); 
}

void 
cfg_edge_iter::reset(void) 
{
    this->curr_node = 0; 
    this->curr_index = -1; 
}

boolean 
cfg_edge_iter::is_empty() const
{
    return (unsigned)curr_node >= base->num_nodes(); 
}

void 
cfg_edge_iter::step() 
{
    ++curr_index; 
    cfg_node *nod = base->node(curr_node); 
    while ((multigraph && curr_index >= nod->succs()->count()) ||
	   (!multigraph && curr_index >= nod->preds()->count())) {
	++curr_node; 
	if ((unsigned)curr_node >= base->num_nodes())
	    break;
	curr_index = 0; 
	nod = base->node(curr_node);
    }
}

cfg_node *
cfg_edge_iter::curr_head(void) const
{
    if (multigraph)
	return base->node(curr_node); 
    else
	return (*base->node(curr_node)->preds())[curr_index]; 
}

cfg_node *
cfg_edge_iter::curr_tail(void) const
{
    if (multigraph)
	return (*base->node(curr_node)->succs())[curr_index]; 
    else
	return base->node(curr_node); 
}

unsigned 
cfg_edge_iter::curr_succ_num(void) const
{
    assert(multigraph); 
    return (unsigned)curr_index; 
}
