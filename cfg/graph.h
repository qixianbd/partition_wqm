/*  Control Flow Graph */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995, 1996, 1997 The President and Fellows of Harvard 
    University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>


#ifndef CFG_GRAPH_H
#define CFG_GRAPH_H

#pragma interface

class cfg_node;
class cfg_block;
class cfg_node_list;

/*
 *  The control flow graph defined here is intended to be general enough
 *  for almost everyone to use (at least as a base -- other information
 *  can be added on top of it).  It supports nodes containing either
 *  individual tree_instrs or basic blocks of contiguous tree_instrs.
 *  High-level tree nodes are represented by multiple flow graph nodes
 *  corresponding to the code that will be created when they are expanded.
 *  Moreover, each high-level tree node is delimited by a pair of BEGIN
 *  and END nodes to make it easier to match up the flow graph with the
 *  SUIF code.
 */

DECLARE_X_ARRAY(cfg_node_array, cfg_node, 10);

class cfg {
    friend class cfg_node;

  private:
    tree_proc *tp;                      /* proc of this cfg */

    cfg_node_array *nds;                /* xarray of nodes */
    cfg_node *en;                       /* entry node */
    cfg_node *ex;                       /* exit node */

    bit_set *doms, *pdoms;              /* dominators and postdominators */
    cfg_node **idom, **ipdom;           /* immediate dominators and postdoms */
    bit_set *df, *rdf;                  /* dominance frontier and reverse df */
    int *lp_depth;                      /* loop depth */

  protected:
    /* internal methods for building the graph */
    void add_node(cfg_node *n);
    cfg_node *graph_add_list(tree_node_list *list, cfg_node *prev,
        boolean block, boolean break_at_call, boolean keep_layout); 
    cfg_node *graph_add_tree(tree_node *t, cfg_node *prev, 
        boolean block, boolean break_at_call, boolean keep_layout); 

    /* helper routine to clone nodes or create new empty nodes */
    void attach(cfg_node *);

    /* dfa functions used internally */
    bit_set *dominators(boolean forward);
    cfg_node **immed_dominators(boolean forward);
    void dom_frontiers(cfg_node *x, boolean forward);

    /* helper routines for clean-up */
    void cfg_cleanup(cfg_node *nd);

  public:
    tree_proc *tproc()                  { return tp; }

    unsigned num_nodes();
    cfg_node *node(unsigned i)          { return (*nds)[i]; }
    cfg_node *&operator[](unsigned i)   { return (*nds)[i]; }

    cfg_node *entry_node()              { return en; }
    cfg_node *exit_node()               { return ex; }
    void set_entry_node(cfg_node *n);
    void set_exit_node(cfg_node *n);

    unsigned num_edges(); 
    unsigned num_multi_edges(); 

    /* node insert/create routine -- must manually set preds/succs */
    cfg_block *clone_into(cfg_block *); 
    cfg_block *new_empty_block(void); 

    /* built-in analysis routines */
    void find_dominators();
    void find_postdominators();
    void find_df();                     /* dominance frontier */
    void find_reverse_df();

    /* access dfa information */
    boolean dominates(int n_dominator, int n_dominatee);
    boolean dominates(cfg_node *dominator, cfg_node *dominatee);
    boolean postdominates(int n_dominator, int n_dominatee);
    boolean postdominates(cfg_node *dominator, cfg_node *dominatee);
    cfg_node *immed_dom(int n);
    cfg_node *immed_dom(cfg_node *n);
    cfg_node *immed_postdom(int n);
    cfg_node *immed_postdom(cfg_node *n);
    bit_set *dom_frontier(int n);
    bit_set *dom_frontier(cfg_node *n);
    bit_set *reverse_dom_frontier(int n);
    bit_set *reverse_dom_frontier(cfg_node *n);

    void find_natural_loops(); 
    int loop_depth(int n);
    int loop_depth(cfg_node *n);
    void set_loop_depth(cfg_node *n, int d); 
    void set_loop_depth(int n, int d); 

    boolean is_loop_begin(int n);       /* TRUE if block is loop entry */
    boolean is_loop_begin(cfg_node *cn);
    boolean is_loop_end(int n);         /* TRUE if block jumps to loop entry */
    boolean is_loop_end(cfg_node *cn);
    boolean is_loop_exit(int n);        /* TRUE if block is a loop exit */ 
    boolean is_loop_exit(cfg_node *cn);

    /* list generator routines */
    cfg_node_list *reverse_postorder_list(boolean forward=TRUE);

    /* clean-up routines */
    boolean remove_unreachable_blocks(boolean delete_instrs=FALSE); 
    boolean merge_block_sequences(boolean break_at_call=FALSE);
    boolean optimize_jumps(); 
    void remove_shadows(); 
    void remove_layout_info(); 

    void print(FILE *fp, boolean show_layout=FALSE);
    void print_with_instrs(FILE *fp, boolean show_layout=FALSE);
    cfg(tree_block *b,
        boolean build_blocks = TRUE,    /* if FALSE, 1 instr per block */
        boolean break_at_call = FALSE, 
        boolean keep_layout = FALSE);
    ~cfg();


};

class cfg_edge_iter {
    boolean multigraph; 
    int curr_node; 
    int curr_index; 
    cfg *base; 
public:
    cfg_edge_iter (cfg *base, boolean multigraph = FALSE); 
    void reset(void); 
    boolean is_empty() const; 
    void step(); 

    cfg_node *curr_head(void) const; 
    cfg_node *curr_tail(void) const; 
    unsigned curr_succ_num(void) const; 
}; 

#endif /* CFG_GRAPH_H */

