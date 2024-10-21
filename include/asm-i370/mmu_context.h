#include <linux/config.h>

#ifndef __I370_MMU_CONTEXT_H
#define __I370_MMU_CONTEXT_H

/* XXX TODO Currently unused but perhaps the right way to handle cr1? */

/*
 * Get a new mmu context for task tsk if necessary.
 */
#define get_mmu_context(tsk) do { } while (0)

/*
 * Set up the context for a new address space.
 */
#define init_new_context(mm)	do { } while (0)

#define set_context(context)    do { } while (0)

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
