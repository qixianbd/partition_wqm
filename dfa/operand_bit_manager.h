/*  Operand bit manager interface */

/*  Copyright (c) 1996,1997 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>

#ifndef OPERAND_BIT_MANAGER_H
#define OPERAND_BIT_MANAGER_H
   
class hard_reg_map;

class operand_bit_manager {
  public:
    typedef boolean (*filter_f)(operand);

    operand_bit_manager(filter_f = NULL, hard_reg_map * = NULL,
                        int hash_table_size = 1024,
                        boolean reverse_map = FALSE);
    ~operand_bit_manager();

    int num_bits() { return next_index; }

    boolean enroll(operand, int *index_ptr = NULL, int *count_ptr = NULL);
    void    enroll(instruction *);

    boolean lookup(operand, int *index_ptr = NULL, int *count_ptr = NULL);
    boolean forget(operand);

    boolean insert(operand, bit_set *);
    boolean remove(operand, bit_set *);

    operand retrieve(int index, int skip);
    void print_entries(bit_set *, FILE *);

    boolean intersects(operand, bit_set *);
    boolean dst_intersects(instruction *, bit_set *);
    boolean src_intersects(instruction *, bit_set *);
    
  private:
    filter_f filter;
    int next_index;
    hard_reg_map *hard_map;
    hash_table   *soft_map;
    hash_table   *anti_map;
    hard_reg_map *hard_map_own;
    hash_e *next_soft_map_e;
    hash_e *next_anti_map_e;
    typedef void (bit_set::*bit_set_op)(int);
    boolean insert_or_remove(operand, bit_set *, bit_set_op op);
    boolean get_bit_range(operand, int *, int *, boolean);
    boolean hr_bit_range(operand, int *, int *);
    boolean covers_hr(bit_set *, operand);
};

class hard_reg_map {
  public:
    hard_reg_map();
    hard_reg_map(int capacity);
    ~hard_reg_map();
    inline int length();
    void enter(int reg, int size, boolean overlay = FALSE);
    int index(int reg);
    int size(int reg);

  private:
    int next_index;
    int capacity;
    int *indexes;
    int *sizes;
};

inline int hard_reg_map::length() { return next_index; }

#endif /* OPERAND_BIT_MANAGER_H */
