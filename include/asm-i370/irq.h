#include <linux/config.h>

#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

/* XXX all wrong */
/*
 * this is the # irq's for all ppc arch's (pmac/chrp/prep)
 * so it is the max of them all - which happens to be powermac
 * at present (G3 powermacs have 64).
 */
#define NR_IRQS			128


#define NUM_8259_INTERRUPTS	16
#define NUM_OPENPIC_INTERRUPTS	20
#define is_8259_irq(n)		((n) < NUM_8259_INTERRUPTS)
#define openpic_to_irq(n)	((n)+NUM_8259_INTERRUPTS)
#define irq_to_openpic(n)	((n)-NUM_8259_INTERRUPTS)
#define IRQ_8259_CASCADE	NUM_8259_INTERRUPTS

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

static __inline__ int irq_cannonicalize(int irq)
{
	return irq;
}

#endif