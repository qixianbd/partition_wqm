/* Interface for bit-vector-based reaching-definitions analyzer */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef REACHING_DEF_H
#define REACHING_DEF_H

class reaching_def_problem : protected bit_vector_problem {
  public:
    reaching_def_problem(cfg *, def_teller *);
    virtual ~reaching_def_problem();

    boolean reaching_in(cfg_node *n, int definee);
    boolean reaching_out(cfg_node *n, int definee);
    bit_set *reaching_in_set(cfg_node *n);
    bit_set *reaching_out_set(cfg_node *n);

    def_catalog *catalog();
    bit_set *def_points_for(int definee);

  protected:
    /*
     * Each of the following arrays maps a CFG node to one of the bit sets
     * described in Muchnick 8.1.  E.g., for the i-th block, Muchnick's
     * PRSV(i) is represented by (*PRSV)[i], which points (once
     * initialized) to a bit set with one bit per definition point.
     */
    bit_set_array *PRSV;
    bit_set_array *GEN;
    bit_set_array *RCH_in;
    bit_set_array *RCH_out;

    /* 
     * Methods common to all bit_vector_problem's
     */
    void build_sets();
    boolean solver_ops(cfg_node *);

    void compose_transfer(cfg_node *, bit_set_array *, bit_set_array *);
    void catalog_def_points(cfg *, def_teller *);

    def_teller  *the_teller;
    def_catalog *the_catalog;

    /*
     * Map from definee id to the list of definition points of the definee
     * (used when collecting these points).
     */
    DECLARE_LIST_CLASS(def_point_list, int);
    DECLARE_X_ARRAY(def_point_list_array, def_point_list, 1000);

    def_point_list_array *def_point_lists;

    /* Map from definee to the (bit) set of definition points of the definee
     * (used by the public def_points_for() method).
     */
    bit_set_array *def_point_sets;

    void delete_def_point_list_array(def_point_list_array *);  // cleanup helper

private:
    bit_set *empty_set;					       // default result
};


/*
 * Annotation mapping an instruction to a list of the ids of its definees
 */

extern char *k_definee_ids;

#endif /* REACHING_DEF_H */
