#include <linux/config.h>

#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include <asm/unitblk.h>
#include <asm/ptrace.h>

#define NR_IRQS                       128

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

void
do_IRQ(int irq, struct pt_regs * regs, unitblk_t *ucb);

static __inline__ int 
irq_cannonicalize(int irq)
{
	return irq;
}

#endif
