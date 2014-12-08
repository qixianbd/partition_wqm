/*  Control Flow Graph Utility Implementations */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#define _MODULE_ "libcfg.a"

#include <suif1.h>
#include <machine.h>
#include "cfg.h"


/*  annotation names */
char *k_cfg_node;
char *k_cfg_begin;
char *k_cfg_end;
char *k_cfg_test;
char *k_cfg_toplab;


/*
 *  Initialization and finalization functions for the CFG library.
 */

void
init_cfg (int & /* argc */, char * /* argv */ [])
{
    /* control flow graph annotations are unregistered */
    k_cfg_node = lexicon->enter("cfg_node")->sp;
    k_cfg_begin = lexicon->enter("cfg_begin")->sp;
    k_cfg_end = lexicon->enter("cfg_end")->sp;
    k_cfg_test = lexicon->enter("cfg_test")->sp;
    k_cfg_toplab = lexicon->enter("cfg_toplab")->sp;
}


void
exit_cfg ()
{
}


/*
 *  Retrieve the node corresponding to the position of a label in a control
 *  flow graph.  This function will return NULL if the flow graph has not yet
 *  been constructed.
 */

cfg_node *
label_cfg_node (label_sym *l)
{
    if (l->are_annotations()) {
	annote *an = l->annotes()->peek_annote(k_cfg_node);
	if (an) return (cfg_node *)an->data();
    }
    return NULL;
}


/*
 *  Retrieve the nodes corresponding to the beginning or end of an AST node
 *  in a control flow graph.  For instruction nodes, the result is the
 *  flow graph node that holds the instruction; for other nodes, it is
 *  the begin/end marker node.  These functions will return NULL if the
 *  flow graph has not yet been constructed.
 */

cfg_node *
cfg_begin_node (tree_node *n)
{
    char *an_name = k_cfg_begin;
    if (n->is_instr()) an_name = k_cfg_node;

    if (n->are_annotations()) {
	annote *an = n->annotes()->peek_annote(an_name);
	if (an) return (cfg_node *)an->data();
    }
    return NULL;
}


cfg_node *
cfg_end_node (tree_node *n)
{
    char *an_name = k_cfg_end;
    if (n->is_instr()) an_name = k_cfg_node;

    if (n->are_annotations()) {
	annote *an = n->annotes()->peek_annote(an_name);
	if (an) return (cfg_node *)an->data();
    }
    return NULL;
}


/*
 *  Retrieve the "test" and "top" nodes for a FOR loop in a control flow
 *  graph.  The separate node for the invisible "top" label is only used
 *  when the graph is built with nodes for individual instructions instead
 *  of basic blocks.  These functions will return NULL if the flow graph has
 *  not yet been constructed.
 */

cfg_test *
cfg_test_node (tree_for *n)
{
    if (n->are_annotations()) {
	annote *an = n->annotes()->peek_annote(k_cfg_test);
	if (an) return (cfg_test *)an->data();
    }
    return NULL;
}


cfg_label *
cfg_toplab (tree_for *n)
{
    if (n->are_annotations()) {
	annote *an = n->annotes()->peek_annote(k_cfg_toplab);
	if (an) return (cfg_label *)an->data();
    }
    return NULL;
}

/*
 * Generate vcg format file for viewing with xvcg
 */

void
generate_vcg(FILE *f, cfg *cfg)
{
    /* Use minbackward layout algorithm in an attempt to make all backedges  */
    /* true backedges. Doesn't always work, but the back algorithm for this. */
    /* Change these parameters as needed, but they have worked fine so far.  */
    fprintf(f,"graph: { title: \"CFG_GRAPH\"\n");
    fprintf(f,"\n");    
    fprintf(f,"x: 30\n");
    fprintf(f,"y: 30\n");
    fprintf(f,"height: 800\n");
    fprintf(f,"width: 500\n");
    fprintf(f,"stretch: 60\n");
    fprintf(f,"shrink: 100\n");
    fprintf(f,"layoutalgorithm: minbackward\n");
    fprintf(f,"node.borderwidth: 3\n");
    fprintf(f,"node.color: white\n");
    fprintf(f,"node.textcolor: black\n");
    fprintf(f,"node.bordercolor: black\n");
    fprintf(f,"edge.color: black\n");
    fprintf(f,"\n");

    /* Put down nodes */
    int num = cfg->num_nodes();	
    int i; 
    for (i=0; i<num; i++)
	fprintf(f,"node: { title:\"%d\" label:\"%d\" }\n", \
		cfg->node(i)->number(),cfg->node(i)->number());
    fprintf(f,"\n");

    /* Put down edges and backedges */
    for (i=0; i<num; i++) {
	cfg_node_list_iter cfgnli(cfg->node(i)->preds());
	cfg_node *itnd;
	while (!cfgnli.is_empty()) {
	    itnd = cfgnli.step();
	    if (itnd->number()<cfg->node(i)->number())
		fprintf(f,"edge: {sourcename:\"%d\" targetname:\"%d\" }\n",\
			itnd->number(), cfg->node(i)->number());
	    else
		fprintf(f,"backedge: {sourcename:\"%d\" targetname:\"%d\" }\n"\
			,itnd->number(), cfg->node(i)->number());
    }
    }

    /* Wrap up at the end */
    fprintf(f,"\n");
    fprintf(f, "}\n");
}


/*
 * cfg_node_instr_iter - iterator over cfg_block instructions
 */

/* Constructor */

cfg_node_instr_iter::cfg_node_instr_iter(cfg_node *nod, boolean reverse)
{
    if (nod->is_block()) {
	cfg_block *blk = (cfg_block *)nod; 
	first = blk->in_head()->list_e();
	last = blk->in_tail()->list_e();
    } else if (nod->is_instr()) {
	cfg_instr *ins = (cfg_instr *)nod; 
	first = ins->instr()->list_e(); 
	last = ins->instr()->list_e(); 
    } else {
	assert_msg(FALSE, ("cfg_node_instr_iter -- Unimplemented node type")); 
    }
    sentinel = reverse ? first : last; 
    rev = reverse; 
    reset(); 
}

/* Reset the iter to the beginning of the basic block */

void
cfg_node_instr_iter::reset()
{
    cur = NULL;
    nxt = rev ? last : first;
}

/* Step the iter to the next instruction */

tree_instr *
cfg_node_instr_iter::step()
{
    /* Return NULL if at end of basic block */
    if (cur == sentinel)
	return NULL;

    cur = nxt;
    nxt = rev ? nxt->prev() : nxt->next();
    return (tree_instr *)cur->contents;
}

/* Look at the next instruction, but don't step the iter */

tree_instr *
cfg_node_instr_iter::peek()
{
    /* Return NULL if at end of basic block */
    if (cur == sentinel)
	return NULL;

    return (tree_instr *)nxt->contents;
}

/* Am I at the end of the instructions in this basic block? */

boolean
cfg_node_instr_iter::is_empty()
{
    return (cur == sentinel);
}

/**
 ** ------------- Helper functions ------------- 
 **/

/* occurs() -- returns TRUE if cn is an element of cnl */ 
boolean
occurs (cfg_node *cn, cfg_node_list *cnl)
{
    cfg_node_list_iter pred_iter(cnl); 
    while (!pred_iter.is_empty()) 
	if (pred_iter.step() == cn) 
	    return TRUE; 
    return FALSE; 
}


/*
 * expunge_instr_tree -- From block b, excise and delete instruction node ti and
 * all the instructions subtended by it.  Do not assume the reachable
 * instructions are in subtrees under ti; in low SUIF, they may be parented by
 * other tree_instr's in block b.
 *
 * To be sure that b's head and tail pointers are maintained correctly, insert
 * temporary head and tail instructions, so that no excision affects
 * b->in_head() or b->in_tail().  Discard these temporary instructions after
 * deletion is done.
 *
 * To extract and delete the tree, use an obscure fact about class
 * tree_node_list: its destructor converts a list instance to expression tree
 * form before deleting it.  And the subexpressions thus sucked into the tree
 * don't have to be parented within the same tree_node_list!
 */

void
expunge_instr_tree(cfg_block *b, tree_instr *ti)
{
    tree_instr *front_bumper = new tree_instr(New_null());
    tree_instr *rear_bumper  = new tree_instr(New_null());
    b->push  (new tree_node_list_e(front_bumper));
    b->append(new tree_node_list_e(rear_bumper));
    {
	// tree_node_list::~tree_node_list() calls cvt_to_trees() before
	// expunging all nodes in the list.  So on exit from this inner (C)
	// block, it collects all instructions reached from ti and deletes
	// them all at once.

	tree_node_list trash;
	trash.append(b->remove(ti->list_e()));
    }
    delete b->pop();
    delete b->remove(b->in_tail()->list_e());
    delete front_bumper;
    delete rear_bumper;
}


/* --- helpers added by TJC --- */

instruction *
New_lab(label_sym *l)
{
instruction *i=NULL;

    if (target_arch->family() == k_suif) {
        i = new in_lab(l);
    } else {
        i = new mi_lab(Label_op(target_arch), l);
    }
    return i;
}


instruction *
New_ubr(label_sym *t)
{
instruction *i=NULL;

    if (target_arch->family() == k_suif) {
        i = new in_bj(io_jmp, t);
    } else {
        i = new mi_bj(Ubr_op(target_arch), t);
    }
    return i;
}


instruction *
New_null()
{
instruction *i=NULL;

    if (target_arch->family() == k_suif) {
        i = new in_rrr(io_nop);
    } else {
	i = new mi_xx(Null_op(target_arch));
    }
    return i;
}
