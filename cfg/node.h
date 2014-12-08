/*  Control Flow Graph Nodes */

/*  Copyright (c) 1994 Stanford University

    All rights reserved.

    Copyright (c) 1995, 1996, 1997 The President and Fellows of Harvard 
    University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>


#ifndef CFG_NODE_H
#define CFG_NODE_H

#pragma interface


/*
 *  Control flow graph nodes are divided into several subclasses
 *  depending on the sort of SUIF object(s) to which they correspond.
 */

enum cfg_node_kind {
    CFG_BEGIN,                          /* beginning marker for AST node */
    CFG_END,                            /* ending marker for AST node */
    CFG_INSTR,                          /* instruction or expression tree */
    CFG_BLOCK,                          /* basic block of instructions */
    CFG_TEST,                           /* "test" node of a FOR loop */
    CFG_LABEL                           /* implicit AST node label */
};

/*
 *  Each flow graph node contains only an ID number and information
 *  related to the flow graph.  Other information can be associated with
 *  a node by storing it in a separate array in the cfg structure where
 *  the node ID number is used to index into the array.
 *
 *  This is a virtual class that cannot be instantiated.
 */

class cfg_node;
DECLARE_LIST_CLASS(cfg_node_list, cfg_node*);

class cfg_node {
    friend class cfg;

  private:
    unsigned nnum;                      /* node number */
    cfg *par;                           /* parent flow graph */
    cfg_node_list prs;                  /* cfg predecessors */
    cfg_node_list scs;                  /* cfg successors */

  protected:
    cfg_node *lpred;                    /* layout predecessor */
    cfg_node *lsucc;                    /* layout successor */

    void set_number(unsigned n)         { nnum = n; }
    void set_parent(cfg *p)             { par = p; }

    void add_succ(cfg_node *n);
    void remove_pred(cfg_node *n);
    void remove_succ(cfg_node *n);

    void print_base(FILE *fp);

  public:
    cfg_node();
    virtual ~cfg_node() {}

    virtual cfg_node_kind kind() = 0;
    boolean is_begin()                  { return (kind() == CFG_BEGIN); }
    boolean is_end()                    { return (kind() == CFG_END); }
    boolean is_instr()                  { return (kind() == CFG_INSTR); }
    boolean is_block()                  { return (kind() == CFG_BLOCK); }
    boolean is_test()                   { return (kind() == CFG_TEST); }
    boolean is_label()                  { return (kind() == CFG_LABEL); }

    unsigned number()                   { return nnum; }
    cfg *parent()                       { return par; }

    cfg_node_list *preds()              { return &prs; }
    cfg_node_list *succs()              { return &scs; }
    cfg_node *fall_succ()               { return (*succs())[0]; }
    cfg_node *take_succ()               { return (*succs())[1]; }

    cfg_node *layout_pred() const       { return lpred; }
    cfg_node *layout_succ() const       { return lsucc; }

    virtual void set_succ(unsigned n, cfg_node *) = 0; 
    void set_fall_succ(cfg_node *nod)   { set_succ(0, nod); }
    void set_take_succ(cfg_node *nod)   { set_succ(1, nod); }

    virtual void clear_layout_succ();
    virtual boolean set_layout_succ(cfg_node *);
    boolean set_layout_succ(label_sym *); 

    cfg_block *insert_empty_block(unsigned n); 
    cfg_block *insert_empty_block(cfg_node *dst); 

    virtual void print(FILE *fp=stdout) = 0;
};

/*
 *  The BEGIN and END flow graph nodes are used to indicate the beginning
 *  and ending of tree_nodes (except for tree_instrs).
 */

class cfg_marker : public cfg_node {
    friend class cfg;

  private:
    tree_node *tn;

  public:
    cfg_marker(tree_node *t)            { tn = t; }

    tree_node *node()                   { return tn; }
};

class cfg_begin : public cfg_marker {
  public:
    cfg_begin(tree_node *t) : cfg_marker(t) { }

    cfg_node_kind kind()                { return CFG_BEGIN; }

    void set_succ(unsigned n, cfg_node *); 
    boolean set_layout_succ(cfg_node *); 

    void print(FILE *fp=stdout);
};

class cfg_end : public cfg_marker {
  public:
    cfg_end(tree_node *t) : cfg_marker(t) { }

    cfg_node_kind kind()                { return CFG_END; }

    void set_succ(unsigned, cfg_node *) { assert(0); }

    void print(FILE *fp=stdout);
};

/*
 *  LABEL nodes are only used in flow graphs built with separate nodes for
 *  each instruction (i.e. no basic blocks).  They are used for the implicit
 *  labels associated with high-level AST nodes.  The result is that only
 *  nodes associated with labels will have multiple predecessors -- in other
 *  words, any node with multiple predecessors will be empty.  That is a
 *  nice property for solving data flow problems.
 */

class cfg_label : public cfg_marker {
  public:
    cfg_label(tree_node *t) : cfg_marker(t) { }

    cfg_node_kind kind()                { return CFG_LABEL; }

    void set_succ(unsigned, cfg_node *) { assert(0); }

    void print(FILE *fp=stdout);
};

/*
 *  The CFG_INSTR flow graph nodes correspond to individual SUIF instructions
 *  (not basic blocks).
 */

class cfg_instr : public cfg_node {
    friend class cfg;

  private:
    tree_instr *ti;

  public:
    cfg_instr(tree_instr *t)            { ti = t; }

    cfg_node_kind kind()                { return CFG_INSTR; }

    tree_instr *instr()                 { return ti; }
    void set_succ(unsigned, cfg_node *) { assert(0); }

    void print(FILE *fp=stdout);
};

/*
 *  The BLOCK flow graph nodes represent basic blocks.  Their boundaries
 *  are identified by the starting and ending tree_instr nodes.  Only other
 *  tree_instrs should be in the block -- no high-level tree_nodes.
 */
typedef tree_node_list_e tnle;  /* just shorthand for this declaration */

class cfg_block : public cfg_node {
    friend class cfg;

  private:
    tree_node_list *tnl; 
    tree_instr *ti1;
    tree_instr *ti2;
    tree_instr *tiC;            /* CTI instruction, if any */
    tree_instr *tiS;            /* shadow goto, if needed */

  protected:
    void extend(tree_instr *t)          { ti2 = t; }
    void remove_explicit_targets(); 
    cfg_block *tnl_pred(void); 
    cfg_block *tnl_succ(void); 

  public:
    cfg_block(tree_node_list *tn, tree_instr *t1, tree_instr *t2)
        { tnl = tn; ti1 = t1; ti2 = t2; tiC = NULL; tiS = NULL; }
    cfg_block(cfg_block *orig, base_symtab *dst_scope=NULL); 
    cfg_block(block_symtab *dst_scope); 

    cfg_node_kind kind()                { return CFG_BLOCK; }

    tree_instr *in_head() const         { return ti1; }
    tree_instr *in_tail() const         { return ti2; }
    tree_instr *in_cti() const          { return tiC; }
    tree_instr *in_shad() const         { return tiS; }
    void set_in_head(tree_instr *ti)    { ti1 = ti; } 
    void set_in_tail(tree_instr *ti)    { ti2 = ti; } 
    void set_in_cti(tree_instr *ti)     { tiC = ti; } // CLIFF: need more here

    void clear_layout_succ();
    void set_succ(unsigned n, cfg_node *); 
    boolean set_layout_succ(cfg_node *); 

    /* Opcode-changing utility routine.  Does not change succs. */
    void invert_branch(); 
    void promote_shadow(void); 
    void set_shadow(cfg_node *); 
    label_sym *get_label(); 

    /* Information routines to assist in code layout. */
    boolean ends_in_cti() const         { return tiC != NULL; }
    boolean ends_in_ubr() const;
    boolean ends_in_cbr() const;
    boolean ends_in_mbr() const;
    boolean ends_in_call() const;
    boolean ends_in_return() const;
        
    tnle *first_non_label() const;
    tnle *first_active_op() const;
    tnle *last_non_cti() const;

    /* Routines to add/delete non-CTI instructions to/from a block.
     * Modeled on tree_node_list routines (dlist.h). */
    tnle *push(tnle *new_e);    /* new_e inserted before head */
    tnle *pop();                /* pop first inst in block */
    tnle *append(tnle *new_e);  /* new_e inserted after tail */
    tnle *insert_before(tnle *e, tnle *pos);
    tnle *insert_after(tnle *e, tnle *pos);
    void insert_after(tree_node_list *mtnl, tnle *pos);
    tnle *remove(tnle *rem);    /* remove this from list */
    boolean contains(tnle *test); /* TRUE if test is in this */

    void print(FILE *fp=stdout);
    void print_with_instrs(FILE *fp=stdout);
};

/*
 *  The test part of a tree_for node is generated automatically when the
 *  tree_for is expanded, yet it must still be represented in flow graphs.
 *  The TEST flow graph nodes are used for this special purpose.
 */

class cfg_test : public cfg_node {
    friend class cfg;

  private:
    tree_for *tf;

  public:
    cfg_test(tree_for *t)               { tf = t; }

    cfg_node_kind kind()                { return CFG_TEST; }

    tree_for *for_loop()                { return tf; }
    void set_succ(unsigned, cfg_node *) { assert(0); }

    void print(FILE *fp=stdout);
};


#endif /* CFG_NODE_H */

