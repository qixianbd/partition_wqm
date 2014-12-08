/*  Control Flow Graph Utilities */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995, 1996, 1997 The President and Fellows of Harvard 
    University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>


#ifndef CFG_UTIL_H
#define CFG_UTIL_H

/*  initialization and finalization functions */
void init_cfg(int &argc, char *argv[]);
void exit_cfg();

/*  functions to find CFG nodes associated with various objects */
cfg_node *label_cfg_node(label_sym *l);
cfg_node *cfg_begin_node(tree_node *n);
cfg_node *cfg_end_node(tree_node *n);
cfg_test *cfg_test_node(tree_for *n);
cfg_label *cfg_toplab(tree_for *n);

/*  annotations to attach CFGs to SUIF objects */
EXPORTED_BY_CFG char *k_cfg_node;
EXPORTED_BY_CFG char *k_cfg_begin;
EXPORTED_BY_CFG char *k_cfg_end;
EXPORTED_BY_CFG char *k_cfg_test;
EXPORTED_BY_CFG char *k_cfg_toplab;

extern void generate_vcg(FILE *f, cfg *cfg);    /* Dump vcg format file   */

/* Iterator over instructions in a cfg_block */
class cfg_node_instr_iter {
  private:
    tree_node_list_e *first;
    tree_node_list_e *last;
    tree_node_list_e *sentinel;
    tree_node_list_e *cur;
    tree_node_list_e *nxt;
    boolean rev; 

  public:
    cfg_node_instr_iter(cfg_node *node, boolean reverse=FALSE);

    void reset();
    tree_instr *step();
    tree_instr *peek();

    boolean is_empty();
};

/* Helper functions */
extern boolean occurs(cfg_node *cn, cfg_node_list *cnl); 
extern void expunge_instr_tree(cfg_block *b, tree_instr *ti); 

/* Helper functions added by tjc */
instruction *New_lab(label_sym *l);
instruction *New_ubr(label_sym *l);
instruction *New_null();

#endif /* CFG_UTIL_H */

