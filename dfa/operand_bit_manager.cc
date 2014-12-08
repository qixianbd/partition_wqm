/*  Operand bit manager implementation */

/*  Copyright (c) 1996 The President and Fellows of Harvard University

    All rights reserved.

    This software is provided under the terms described in
    the "suif_copyright.h" include file. */

#include <suif_copyright.h>
#include <suif1.h>
#include <machine.h>
#include <cfg.h>
#include "dfa.h"

/* soft_map_e -- Record the bit index of a non-HR operand */
class soft_map_e : public hash_e {
public:
    int bit_index;
    soft_map_e(unsigned s, int bi) : hash_e(s) { bit_index = bi; }
    soft_map_e &operator=(const soft_map_e &other)
    {
	next_e = other.next_e;
	signature = other.signature;
        bit_index = other.bit_index;
        return *this;
    }
};

DECLARE_LIST_CLASS(opd_list, operand);

/* anti_map_e -- Record the operand for a given bit index */
class anti_map_e : public hash_e {
public:
    opd_list opds;
    anti_map_e(unsigned s) : hash_e(s) { opds.push(operand()); }

    void fill(unsigned s, const operand &opd)
        { signature = s; opds.head()->contents = opd; }
};

/*
 * class anti_map_t -- Hash table whose enter method maintains the operand
 * sets in table entries.  CAUTION: enter isn't a virtual method, so we're
 * relying on the fact that enter isn't needed by other hash table methods.
 */
class anti_map_t : public hash_table {
  public:
    anti_map_t(hash_compare f, unsigned sz) : hash_table(f, sz) { }
    boolean enter(anti_map_e *);
};

boolean
anti_map_t::enter(anti_map_e *new_e)
{
    boolean used_new_e = hash_table::enter(new_e);
    if (!used_new_e) {
	assert(new_e->opds.count() == 1);

	anti_map_e *old_e = (anti_map_e *)lookup(new_e);

	operand opd = new_e->opds[0];
	if (!old_e->opds.lookup(opd))
	    old_e->opds.append(opd);
    }
    return used_new_e;
}

/* Hash table comparators */
boolean
same_soft_map_e(soft_map_e *e1, soft_map_e *e2)
{
    return e1->signature == e2->signature;
}

boolean
same_anti_map_e(anti_map_e *e1, anti_map_e *e2)
{
    return e1->signature == e2->signature;
}


/*
 * Construct an operand bit manager.
 * The bit set space for hardware registers is reserved here.
 * The positions of other operands are allocated above that reserved space
 * as they are encountered.
 * Data member `next_soft_map_e' holds a free hash table entry to be installed
 * (and replaced) next time a new item is added to the `soft_map' table.
 * Data member `next_anti_map_e' does the same for the `anti_map' table.
 */
operand_bit_manager::operand_bit_manager
    (filter_f filter_fun, hard_reg_map *hr_map, int hash_map_size,
     boolean reverse_map)
{
    filter = filter_fun;
    if (hr_map)
	hard_map_own = NULL;
    else
	hard_map_own = hr_map = new hard_reg_map();
    next_index = hr_map->length();
    hard_map = hr_map;
    soft_map = new hash_table((hash_compare)same_soft_map_e, hash_map_size);
    next_soft_map_e = (hash_e *)new soft_map_e(0, 0);

    if (reverse_map) {
	anti_map = new anti_map_t((hash_compare)same_anti_map_e, hash_map_size);
	next_anti_map_e = (hash_e *)new anti_map_e(0);
    } else {
	anti_map = NULL;
	next_anti_map_e = NULL;
    }
}

operand_bit_manager::~operand_bit_manager()
{
    delete hard_map_own;
    delete soft_map;
    delete (soft_map_e *)next_soft_map_e;
    delete anti_map;
    delete (anti_map_e *)next_anti_map_e;
}

boolean
operand_bit_manager::enroll(operand opd, int *index_ptr, int *count_ptr)
{
    return get_bit_range(opd, index_ptr, count_ptr, TRUE);
}

/* enroll(instruction *) -- Put all suitable operands of an instruction under
 * this manager.  */
void
operand_bit_manager::enroll(instruction *gi)
{
    unsigned i;

    for (i = 0; i < gi->num_srcs(); i++) {
	operand s = gi->src_op(i);
	if (s.is_instr())
	    enroll(s.instr());
	else
	    enroll(s);
    }

    for (i = 0; i < gi->num_dsts(); i++)
	enroll(gi->dst_op(i));
}

boolean
operand_bit_manager::lookup(operand opd, int *index_ptr, int *count_ptr)
{
    return get_bit_range(opd, index_ptr, count_ptr, FALSE);
}

operand
operand_bit_manager::retrieve(int index, int skip)
{
    if (anti_map == NULL)
	return operand();

    next_anti_map_e->signature = (unsigned)index;

    if (anti_map_e *ame = ((anti_map_e *)anti_map->lookup(next_anti_map_e)))

	for (opd_list_iter oli(&ame->opds); !oli.is_empty(); --skip, oli.step())
	    if (skip == 0)
		return oli.step();

    return operand();	
}


static unsigned soft_map_key(operand);

boolean
operand_bit_manager::forget(operand opd)
{
    int index, count;
    if (!get_bit_range(opd, &index, &count, FALSE))
	return FALSE;

    if (!opd.is_hard_reg()) {
	next_soft_map_e->signature = soft_map_key(opd);
	assert(next_soft_map_e->signature != 0);

	delete (soft_map_e *)soft_map->remove(next_soft_map_e);
    }
    if (anti_map) {
	next_anti_map_e->signature = index;
	delete (anti_map_e *)anti_map->remove(next_anti_map_e);
    }
    return TRUE;
}


/* get_bit_range -- Look up or allocate a bit range for an operand,
 * returning true iff "successful".
 * Specifically, if `enroll' is true, then return true iff the operand is
 * not filtered out, not a hard register (HR), and produces a new entry
 * when enrolled in soft_map.  If `enroll' is false, then if the operand is
 * an HR for which hard_map gives no range, or if it's filtered out, or if
 * it's neither a virtual register (VR) nor symbol, or if no entry for the
 * operand exists in soft_map, return false.
 * As a side effect when a bit range is found for the operand, store its
 * starting index indirectly via `index_ptr' and its extent via
 * `count_ptr', but omit either store if the respective pointer is null. */

// Cheesy work-around to a compiler bug in VC5. (jsimmons)
typedef boolean (*filter_f)(operand);
boolean do_filter(filter_f filter, operand opd)
{
    return filter(opd);
}

boolean
operand_bit_manager::get_bit_range(operand opd, int *index_ptr, int *count_ptr,
				   boolean enroll)
{
//    if (filter && !filter(opd))	-- see above (jsimmons)
    if (filter && !do_filter(filter, opd))
	return FALSE;				// not a candidate operand

    if (opd.is_hard_reg()) {
	int index;
	boolean valid = hr_bit_range(opd, &index, count_ptr);
	if (index_ptr)
	    *index_ptr = index;
	if (valid && enroll && anti_map) {
	    ((anti_map_e *)next_anti_map_e)->fill(index, opd);
	    if (!anti_map->enter(next_anti_map_e))	// new anti_map entry
		next_anti_map_e = new anti_map_e(0);
	}
	return valid && !enroll;
    }

    unsigned signature = soft_map_key(opd);
    if (!signature)
        return FALSE;

    *(soft_map_e *)next_soft_map_e = soft_map_e(signature, next_index);

    if (enroll && !soft_map->enter(next_soft_map_e)) {	// new soft_map entry
	if (index_ptr)
	    *index_ptr = next_index;
	if (count_ptr)
	    *count_ptr = 1;

	if (anti_map) {
	    ((anti_map_e *)next_anti_map_e)->fill(next_index, opd);

	    boolean found = anti_map->enter(next_anti_map_e);
	    assert(!found);

	    next_anti_map_e = new anti_map_e(0);
	}
	next_soft_map_e = new soft_map_e(0, 0);
	next_index++;
	return TRUE;
    }

    if (enroll && !index_ptr && !count_ptr)
	return FALSE;
    
    soft_map_e *sme = ((soft_map_e *)soft_map->lookup(next_soft_map_e));
    if (!sme)
	return FALSE;
    if (index_ptr)
	*index_ptr = sme->bit_index;
    if (count_ptr)
	*count_ptr = 1;
    return !enroll;
}

/* hr_bit_range -- Helper for get_bit_range.  Lookup an HR operand in a
 * hard_reg_map.  Return false if it has no bit range.  Otherwise, return
 * true and also store the start (length) of the bit range indirectly via
 * index_ptr (count_ptr), provided the respective argument is not null. */

boolean
operand_bit_manager::hr_bit_range(operand opd, int *index_ptr, int *count_ptr)
{
    int index = hard_map->index(opd.reg());

    if (index < 0)
	return FALSE;
    if (index_ptr)
	*index_ptr = index;

    if (count_ptr) {
	int count = 1;				/* no empty bit ranges */
	int unit_size = hard_map->size(opd.reg());
	assert_msg(unit_size > 0, ("operand_bit_manager::hr_bit_range() -- "
				   "no natural size for machine register"));

	int total = opd.type()->size();
	if (total == 0) {	/* opd type void => use natural reg width */
	    int bank = target_regs->describe(opd.reg()).bank;
	    total = target_regs->width_of(bank);
	}
	while ((total -= unit_size) > 0)
	    count++;
	*count_ptr = count;
    }
    return TRUE;
}

/* soft_map_key -- Helper for get_bit_range.  Return zero if the given
 * operand is not a VR or var_sym.  Otherwise, return a unique non-zero
 * hash table key for it.  For a virtual register with number vr
 * (necessarily negative), the key is 2*vr.  For a symbol, it's the odd
 * number equal to or one beyond the symbol's address. */

static unsigned
soft_map_key(operand opd)
{
    unsigned signature;

    if  (opd.is_virtual_reg()) {
	signature = (unsigned)(opd.reg() << 1);
    } else if (opd.is_symbol()) {
	signature = (unsigned long)opd.symbol() | 1;
    } else
	return 0;
    return signature;
}

/* insert -- Insert the bits for an operand in a bit set.
 * If this operand has no corresponding bit range, return FALSE.  */
boolean
operand_bit_manager::insert(operand opd, bit_set *bs)
{
    return insert_or_remove(opd, bs, &bit_set::add);
}

/* remove -- Remove the bits for an operand in a bit set.
 * If this operand has no corresponding bit range, return FALSE.  */
boolean
operand_bit_manager::remove(operand opd, bit_set *bs)
{
    return insert_or_remove(opd, bs, &bit_set::remove);
}

/* insert_or_remove -- Perform operand_bit_manager::insert or ...:: remove.
 * Third operand `op' is a pointer to bit_set::add or bit_set::remove, resp.
 */
boolean
operand_bit_manager::insert_or_remove(operand opd, bit_set *bs, bit_set_op op)
{
    int index, count;
    if (!lookup(opd, &index, &count))
	return FALSE;

    for ( ; ; ) {
	(bs->*op)(index);		/* add or remove a bit */

	if (--count == 0)
	    break;

	/* Only hard regs have count > 1 */
	int bank = target_regs->describe(opd.reg()).bank;
	int mreg = target_regs->a2m(opd.reg());
	int areg = target_regs->m2a(bank, mreg + 1);
	assert_msg(areg >= 0,
		   ("no successor for abstract register %d", opd.reg()));

	boolean found = lookup(operand(areg, type_void), &index);
	assert_msg(found, ("no index for abstract register %d", areg));
    }
    return TRUE;
}


/* intersects -- Return true iff the bit range of an operand has non-empty
 * intersection with a given bit set. */

boolean
operand_bit_manager::intersects(operand opd, bit_set *bs)
{
    int index, count;

    if (!lookup(opd, &index, &count))
        return FALSE;
    do
        if (bs->contains(index++))
	    return TRUE;
    while (--count);

    return FALSE;
}

boolean
operand_bit_manager::dst_intersects(instruction *gi, bit_set *bs)
{
    unsigned i;

    for(i=0; i<gi->num_dsts(); i++)
    if( intersects(gi->dst_op(i), bs) )
	return TRUE;
    return FALSE;
}

boolean
operand_bit_manager::src_intersects(instruction *gi, bit_set *bs)
{
    unsigned i;

    for(i=0; i<gi->num_srcs(); i++) {
	if( gi->src_op(i).is_instr() ) {
	    if( src_intersects(gi->src_op(i).instr(), bs) )
		return TRUE;
	}
	else if( intersects(gi->src_op(i), bs) )
	    return TRUE;
    }
    return FALSE;
}

void
operand_bit_manager::print_entries(bit_set *bs, FILE *f)
{
    assert(bs);
    char *separator = "";

    fputs("{", f);

    for (bit_set_iter biter(bs); !biter.is_empty(); separator = " ") {
	int n = biter.step();
	for (int i = 0; ; ++i) {
	    operand opd = retrieve(n, i);
	    if (opd.is_null())
		break;
	    if (!opd.is_hard_reg() || covers_hr(bs, opd)) {
		fputs(separator, f);
		opd.print(f);
	    }
	}
    }
    fputs("}\n", f);
}

/*
 * covers_hr -- Helper for print_entries.  Return true iff the given bit
 * set completely covers the index range of a hard-register operand.
 */
boolean
operand_bit_manager::covers_hr(bit_set *bs, operand opd)
{
    assert(bs && opd.is_hard_reg());

    int index, count;
    boolean valid = hr_bit_range(opd, &index, &count);
    assert(valid);

    for ( ; count > 0; --count, ++index)
	if (!bs->contains(index))
	    return FALSE;
    return TRUE;
}


/* Construct an empty hard_reg_map. */
hard_reg_map::hard_reg_map(int cap)
{
    next_index = 0;
    capacity = cap;
    indexes = cap ? new int[cap] : NULL;
    sizes   = cap ? new int[cap] : NULL;
    while (cap-- > 0)
        indexes[cap] = -1;
}

/* Construct a default hard_reg_map from the machine description.  This
 * maps every register to a bit range having a bit for every grain. */

hard_reg_map::hard_reg_map()
{
    next_index = 0;
    capacity = target_regs->total_grains();
    printf("capacity:%d\n",capacity);
    indexes = new int[capacity];
    sizes   = new int[capacity];

    for (int b = 0; b < LAST_REG_BANK; b++) {
	int n = target_regs->num_in(b);
        int ratio = 1;
        if(n > 0){
            int width = target_regs->width_of(b);
            int grain_size = target_regs->grain_size_of(b);
            ratio = width / grain_size;
        }
        assert(!n || next_index == target_regs->start_of(b));
        while (n){
            enter(next_index, target_regs->grain_size_of(b));
            n = n - ratio;
        }
    }
}

hard_reg_map::~hard_reg_map()
{
    delete indexes;
    delete sizes;
}

void hard_reg_map::enter(int reg, int size, boolean overlay)
{
    int bank   = target_regs->describe(reg).bank;
    int width  = target_regs->width_of(bank);
    int grains = width / size;

    assert_msg(width == grains * size,
	       ("hard_reg_map::enter() -- "
		"size per bit must divide register width"));

    if (overlay) {
	next_index -= grains;
	assert_msg(next_index >= 0,
		   ("hard_reg_map::enter() -- "
		    "can't overlay when entering first register"));
    }

    indexes[reg] = next_index;
    sizes[reg] = size;
    next_index += grains;
}

int hard_reg_map::index(int reg)
{
    if (reg >= capacity)
	return -1;
    return indexes[reg];
}

int hard_reg_map::size(int reg)
{
    if (reg >= capacity)
	return -1;
    return sizes[reg];
}
