#include <linux/config.h>

#ifndef __I370_MMU_CONTEXT_H
#define __I370_MMU_CONTEXT_H

/* XXX all wrong for the i370 */

/* the way contexts are handled on the ppc they are vsid's and
   don't need any special treatment right now.
   perhaps I can defer flushing the tlb by keeping a list of
   zombie vsid/context's and handling that through destroy_context
   later -- Cort
 */

#define NO_CONTEXT      0
#define LAST_CONTEXT    0xfffff

/*
 * Allocating context numbers this way tends to spread out
 * the entries in the hash table better than a simple linear
 * allocation.
 */
#define MUNGE_CONTEXT(n)        (((n) * 897) & LAST_CONTEXT)

extern atomic_t next_mmu_context;
extern void mmu_context_overflow(void);
extern void set_context(int context);

/*
 * Get a new mmu context for task tsk if necessary.
 */
#define get_mmu_context(tsk)					\
do { 								\
	struct mm_struct *mm = (tsk)->mm;			\
	if (mm->context == NO_CONTEXT) {			\
		if (atomic_read(&next_mmu_context) == LAST_CONTEXT)		\
			mmu_context_overflow();			\
		mm->context = MUNGE_CONTEXT(atomic_inc_return(&next_mmu_context));\
	}							\
} while (0)

/*
 * Set up the context for a new address space.
 */
#define init_new_context(mm)	((mm)->context = NO_CONTEXT)

/*
 * We're finished using the context for an address space.
 */
#define destroy_context(mm)     do { } while (0)

/*
 * After we have set current->mm to a new value, this activates
 * the context for the new mm so we see the new mappings.
 */
extern inline void activate_context(struct task_struct *tsk)
{
	get_mmu_context(tsk);
	set_context(tsk->mm->context);
}

#endif
