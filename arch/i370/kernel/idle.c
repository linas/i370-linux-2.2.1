/*
 *
 * Idle daemon for I370.  Idle daemon will handle any action
 * that needs to be taken when the system becomes idle.

 * copied from linux/arch/ppc/kernel/idle.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/malloc.h>

#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cache.h>

void zero_paged(void);
unsigned long zero_paged_on = 0;

int idled(void *unused)
{
	/* endless loop with no priority at all */
	current->priority = 0;
	current->counter = -100;
	for (;;)
	{
		__sti();
		
		check_pgt_cache();

/*
 * It might be interesting to copy the zero_paged code from 
 * the powerpc directory to here ... sounds like a good idea ...
		if ( !current->need_resched && zero_paged_on ) zero_paged();
*/

#ifdef __SMP__
		if (current->need_resched)
#endif
			schedule();
	}
	return 0;
}

#ifdef __SMP__
/*
 * SMP entry into the idle task - calls the same thing as the
 * non-smp versions. -- Cort
 */
int cpu_idle(void *unused)
{
	idled(unused);
	return 0; 
}
#endif /* __SMP__ */

/*
 * Syscall entry into the idle task. -- Cort
 */
asmlinkage int sys_idle(void)
{
	if(current->pid != 0)
		return -EPERM;

	idled(NULL);
	return 0; /* should never execute this but it makes gcc happy -- Cort */
}

