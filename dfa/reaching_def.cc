/* Interface for bit-vector-based reaching-definitions analyzer */

/*  Copyright (c) 1998 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"


reaching_def_problem::reaching_def_problem(cfg *cfg_, def_teller *teller_) :
    bit_vector_problem(forward, cfg_)
{
    the_teller  = teller_;
    the_catalog = NULL;
    def_point_lists = NULL;
    def_point_sets  = NULL;
    empty_set = NULL;
    PRSV = GEN = RCH_in = RCH_out = NULL;
    solve();
}

reaching_def_problem::~reaching_def_problem() {
    Delete_bit_set_array(PRSV);
    Delete_bit_set_array(GEN);
    Delete_bit_set_array(RCH_in);
    Delete_bit_set_array(RCH_out);

    Delete_bit_set_array(def_point_sets);
    delete_def_point_list_array(def_point_lists);
    delete empty_set;
//    delete the_catalog;				// FIXME: creates dce bug
}

boolean
reaching_def_problem::reaching_in(cfg_node *n, int definee)
{
    return (*RCH_in)[n->number()]->contains(definee);
}

boolean
reaching_def_problem::reaching_out(cfg_node *n, int definee)
{
    return (*RCH_out)[n->number()]->contains(definee);
}

bit_set *
reaching_def_problem::reaching_in_set(cfg_node *n)
{
    return (*RCH_in)[n->number()];
}

bit_set *
reaching_def_problem::reaching_out_set(cfg_node *n)
{
    return (*RCH_out)[n->number()];
}

def_catalog *
reaching_def_problem::catalog()
{
    return the_catalog;
}

bit_set *
reaching_def_problem::def_points_for(int definee)
{
    if (!def_point_sets) {
        def_point_sets = new bit_set_array();
        for (int i = 0; i < def_point_lists->ub(); i++) {
            def_point_list *list = (*def_point_lists)[i];
            bit_set *set = new bit_set(0, the_catalog->num_defs());

            while (!list->is_empty())
                set->add(list->pop());
            def_point_sets->extend(set);
        }
        delete_def_point_list_array(def_point_lists);
        def_point_lists = NULL;
        empty_set = new bit_set(0, the_catalog->num_defs());
    }
    if (definee >= def_point_sets->ub())
	return empty_set;
    return (*def_point_sets)[definee];
}


char *k_definee_ids;

/*
 * Map from CFG node number to the first possible definition point id
 * within the node.  Used in initialization of GEN[i] to help recognize
 * previous defs in block i that are being superseded.
 */
static int *first_def;


/*
 * reaching_def_problem::build_sets() -- (re)initialize the per-block
 * bit_sets and setup to (re)solve the dataflow equations.
 */
void
reaching_def_problem::build_sets()
{
    first_def = new int[the_cfg->num_nodes()];

    def_point_lists = new def_point_list_array;

    catalog_def_points(the_cfg, the_teller);

    Delete_bit_set_array(PRSV);
    Delete_bit_set_array(GEN);
    Delete_bit_set_array(RCH_in);
    Delete_bit_set_array(RCH_out);

    PRSV    = new bit_set_array();
    GEN     = new bit_set_array();
    RCH_in  = new bit_set_array();
    RCH_out = new bit_set_array();

    for (unsigned i = 0; i < the_cfg->num_nodes(); i++) {
	PRSV   ->extend(new bit_set(0, the_catalog->num_defs(), TRUE));
	GEN    ->extend(new bit_set(0, the_catalog->num_defs(), TRUE));
	RCH_in ->extend(new bit_set(0, the_catalog->num_defs(), TRUE));
	RCH_out->extend(new bit_set(0, the_catalog->num_defs(), TRUE));

        (*PRSV)   [i]->universal();
        (*GEN)    [i]->clear();
        (*RCH_in) [i]->clear();
        (*RCH_out)[i]->clear();

        cfg_node *nd = the_cfg->node(i);
        if (nd->is_begin() || nd->is_end())
            continue;

        assert_msg(nd->is_instr() || nd->is_block(),
                   ("DFA doesn't do high SUIF nodes"));
        
        compose_transfer(nd, PRSV, GEN); 
    }

    delete [] first_def;
}

/*
 * reaching_def_problem::catalog_def_points -- Catalog the definition
 * points in each basic block.  For each basic block, record the first
 * possible definition point id within the block.  For each definee, make a
 * list of its definition point id's.  For each instruction that has
 * definition points, save the list of definees as an annotation.
 */

void
reaching_def_problem::catalog_def_points(cfg *the_cfg, def_teller *the_teller)
{
    delete the_catalog;
    the_catalog = new def_catalog;

    for (unsigned i = 0; i < the_cfg->num_nodes(); i++) {
        cfg_node *nd = the_cfg->node(i);
        if (!nd->is_block() && !nd->is_instr())
            continue;

        int def_point = the_catalog->num_defs();
        first_def[i] = def_point;

        cfg_node_instr_iter ndi(nd);
        while (!ndi.is_empty()) {
            tree_instr *ti = ndi.step();
            instruction *in = ti->instr();
            definee_list *definees = the_teller->definees(in);

            in->prepend_annote(k_definee_ids, (void *)definees);
            the_catalog->enter(in, definees->count());
            
            definee_list_iter di(definees);
            while (!di.is_empty()) {
                int definee = di.step();
                while (definee >= def_point_lists->ub())
                    def_point_lists->extend(new def_point_list);
                (*def_point_lists)[definee]->append(def_point++);
            }
        }
    }
}

/*
 * reaching_def_problem::compose_transfer -- Encapsulate the effects of a
 * single CFG node by scanning its instructions in forward order and
 * adjusting the GEN and PRSV sets (see Muchnick 8.1) to reflect
 * definitions seen.  If a definee of the current instruction has another
 * definition point that lies between the first def point of this node and
 * the first def point of the current instruction, then the prior
 * definition does not reach the end of the node, and it is dropped from
 * the GEN set.
 */

void
reaching_def_problem::compose_transfer
			(cfg_node *nd, bit_set_array *PRSV, bit_set_array *GEN)
{
    int nd_num = nd->number();

    cfg_node_instr_iter ndi(nd);
    while (!ndi.is_empty()) {
        tree_instr *ti = ndi.step();
        instruction *in = ti->instr();
        int cur_def_id = the_catalog->first_id(in);
        if (cur_def_id < 0)
            continue;

        definee_list *definees = (definee_list *) in->get_annote(k_definee_ids);
        assert(definees && definees->count() == the_catalog->num_ids(in));

        definee_list_iter di(definees);
        while (!di.is_empty()) {
            int definee = di.step();
            def_point_list_iter pi((*def_point_lists)[definee]);
            while (!pi.is_empty()) {
                int def_pt = pi.step();
                if (first_def[nd_num] <= def_pt && def_pt < cur_def_id)
                    (*GEN)[nd_num]->remove(def_pt);
                (*PRSV)[nd_num]->remove(def_pt);
            }
        }
        definees->erase();

        for (int i = the_catalog->num_ids(in); i; i--)
            (*GEN)[nd_num]->add(cur_def_id++);
    }
}

/*
 * reaching_def_problem::solver_ops() -- propagates reaching_def
 * dataflow information through one block.  Returns TRUE if the
 * RCH_in set for this block changed.
 */
boolean
reaching_def_problem::solver_ops(cfg_node *nd)
{
    int nd_num = nd->number();
    bit_set *in = (*RCH_in)[nd_num];
    bit_set *out = (*RCH_out)[nd_num];

    /*
     * RCHin(nd) = U RCHout[preds(nd)]
     */
    in->clear();

    cfg_node_list_iter pi(nd->preds());
    while (!pi.is_empty())
	*in += *((*RCH_out)[pi.step()->number()]);

    /*
     * RCHout(nd) = GEN(nd) U (RCHin(nd) intersect PRSV(nd))
     */
    bit_set t;             /* Separate declaration from assignment ... */
    t = *in;		   /* ... till bit_set gets a copy constructor */
    t *= *((*PRSV)[nd_num]);
    t += *((*GEN)[nd_num]);

    /*
     * Update RCHout(nd)  if necessary
     */
    if (t != *out) {
	*out = t;
	return TRUE;
    }
    return FALSE;
}

void
reaching_def_problem::delete_def_point_list_array(def_point_list_array *a)
{
    if (a) {
        for (int i = 0; i < a->ub(); i++)
            delete (*a)[i];
        delete a;
    }
}
