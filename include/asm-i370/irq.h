#include <linux/config.h>

#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#define NR_IRQS                       128

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);


static __inline__ int 
irq_cannonicalize(int irq)
{
	return irq;
}

#endif
