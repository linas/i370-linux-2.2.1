/************************************************************/
/*							  */
/* Module ID  - irq.					*/
/*							  */
/* Function   - Support Interrupts under S/390 using the    */
/*	      interrupt subclass which vaguely equates    */
/*	      to the IRQ mechanism used on other systems. */
/*							  */
/* Called By  - Kernel.				     */
/*							  */
/* Notes      - (1) ....................................... */
/*							  */
/*	      (2) ....................................... */
/*							  */
/* Name       - Neale Ferguson.			     */
/* Date       - August, 1999.			       */
/*							  */
/* Associated    - (1) Refer To ........................... */
/* Documentation					    */
/*		 (2) Refer To ........................... */
/*							  */
/************************************************************/
/************************************************************/
/*							  */
/*		       DEFINES			    */
/*		       -------			    */
/*							  */
/************************************************************/

#define irq_enter(cpu, irq)     (++local_irq_count[cpu])
#define irq_exit(cpu, irq)      (--local_irq_count[cpu])

/*=============== End of Defines ===========================*/

/************************************************************/
/*							  */
/*		INCLUDE STATEMENTS			*/
/*		------------------			*/
/*							  */
/************************************************************/

#include <linux/errno.h>
 #include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/malloc.h>
#include <linux/random.h>

#include <asm/bitops.h>
#include <asm/irq.h>
#include <asm/asm.h>
#include <asm/system.h>
#include <asm/ptrace.h>
#include <asm/unitblk.h>

/*================== End of Include Statements =============*/

/************************************************************/
/*							  */
/*		TYPE DEFINITIONS			  */
/*		----------------			  */
/*							  */
/************************************************************/

/*================== End of Type Definitions ===============*/

/************************************************************/
/*							  */
/*	       FUNCTION PROTOTYPES			*/
/*	       -------------------			*/
/*							  */
/************************************************************/

/*================== End of Prototypes =====================*/

/************************************************************/
/*							  */
/*	     GLOBAL VARIABLE DECLARATIONS		 */
/*	     ----------------------------		 */
/*							  */
/************************************************************/

extern unitblk_t *unit_base;
extern unitblk_t *dev_cons;
extern long      sid_count;

 unsigned int local_bh_count[NR_CPUS];
 unsigned int local_irq_count[NR_CPUS];

struct irqaction *irq_action[NR_IRQS] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

atomic_t __i370_bh_counter;

/*============== End of Variable Declarations ==============*/

/************************************************************/
/*							  */
/* Name       - init_IRQ.				   */
/*							  */
/* Function   - Initialize IRQ processing.		  */
/*							  */
/************************************************************/

__initfunc (void
init_IRQ(void))
{
	cr0_t cr0;
	int i_bh;

	cr0.raw       = _stctl_r0();
	cr0.bits.iksm = ~0;
	_lctl_r0(cr0.raw);
	for (i_bh = 0; i_bh < NR_CPUS; i_bh++) {
		local_bh_count[i_bh] = 0;
		local_irq_count[i_bh] = 0;
	}

}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - disable_irq.				*/
/*							  */
/* Function   - Disable an interrupt subclass.	      */
/*							  */
/************************************************************/

void
irq_init()
 {
	int rc, i_ucb;
	unitblk_t *ucb;

	ucb = unit_base;
	for (i_ucb = 0; i_ucb < sid_count; i_ucb++) {
		if ((ucb[i_ucb].unitstat == UNIT_READY) &&
		    (&ucb[i_ucb] != dev_cons)) {
			rc = request_irq(ucb[i_ucb].unitisc, ucb[i_ucb].unitirqh,
					 SA_INTERRUPT, ucb[i_ucb].unitname,
					 (void *) &ucb[i_ucb]);

			if (rc != 0)
				printk("Unable to request IRQ %d for device %s. rc: %d\n",
				       ucb[i_ucb].unitisc, ucb[i_ucb].unitname, rc);
		}
 	}
 }

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - disable_irq.				*/
/*							  */
/* Function   - Disable an interrupt subclass.	      */
/*							  */
/************************************************************/

void
disable_irq(unsigned int irq_nr)
{
	unsigned char mask;
	cr0_t cr0;

	mask	  = ~(1 << (irq_nr & 7));
	cr0.raw       = _stctl_r0();
	cr0.bits.iksm &= mask;
	_lctl_r0(cr0.raw);
}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - enable_irq.				 */
/*							  */
/* Function   - Enable an interrupt subclass.	       */
/*							  */
/************************************************************/

void
enable_irq(unsigned int irq_nr)
{
	unsigned char mask;
	cr0_t cr0;

	mask	  = 1 << (irq_nr & 7);
	cr0.raw       = _stctl_r0();
	cr0.bits.iksm |= mask;
	_lctl_r0(cr0.raw);
}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - get_irq_list.			       */
/*							  */
/* Function   - Return a list of the interrupt handlers     */
/*	      we have in place.			   */
/*							  */
/************************************************************/

int
get_irq_list(char *buf)
 {
-	int len = 0;
-	len += sprintf(buf+len, "No irqs XXX not implemented\n");
	int i, len = 0;
	struct irqaction * action;

	for (i = 0 ; i < NR_IRQS; i++) {
		action = irq_action[i];
		if (!action)
			continue;
		len += sprintf(buf+len, "%2d: %8d %c %s",
			i, kstat.irqs[0][i],
			(action->flags & SA_INTERRUPT) ? '+' : ' ',
			action->name);
		for (action=action->next; action; action = action->next) {
			len += sprintf(buf+len, ",%s %s",
				(action->flags & SA_INTERRUPT) ? " +" : "",
				action->name);
		}
		len += sprintf(buf+len, "\n");
	}
 	return len;
 }

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - do_IRQ.				     */
/*							  */
/* Function   - Invoke the action routine(s) for a given    */
/*	      device and interrupt subclass.	      */
/*							  */
/************************************************************/

void
do_IRQ(int irq, void *dev_id, struct pt_regs * regs, unitblk_t *ucb)
{
	struct irqaction *action;
	int cpu, do_random;

	cpu = _stap();
	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;

	/*-----------------------------------------------------------*/
	/* Point to the irq_action for this irq		      */
	/*-----------------------------------------------------------*/
	action = ucb->unitaction;
	if (action) {
		do_random = 0;
		do {
			do_random |= action->flags;

			/*---------------------------------------------------------*/
			/* Invoke handler if action is for the interrupting device */
			/*---------------------------------------------------------*/
			if (action->dev_id == (void *) ucb)
				action->handler(irq, action->dev_id, regs);
			action = action->next;
		} while (action);
		/*----------------------------------------------------------*/
		/* Allow our stuff to contribute to the system entropy      */
		/*----------------------------------------------------------*/
		if (do_random & SA_SAMPLE_RANDOM)
			add_interrupt_randomness(irq);
	}
	irq_exit(cpu, irq);

	/* unmasking and bottom half handling is done magically for us. */
}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - request_irq.				*/
/*							  */
/* Function   - Request that a handler be installed for     */
/*	      a given IRQ (and device).		   */
/*							  */
/************************************************************/

int
request_irq(unsigned int irq,
		void (*handler)(int, void *, struct pt_regs *),
		unsigned long irqflags,
		const char * devname,
		void *dev_id)
{
	int retval = 0;
	struct irqaction *action, *old, **p;
	unsigned long flags;
	unitblk_t *ucb;

	if (irq > NR_IRQS)
		retval = -EINVAL;
	else {
		if (!handler)
			retval = -EINVAL;
		else {
			action = (struct irqaction *)kmalloc(sizeof(struct irqaction),
				 GFP_KERNEL);
			if (!action)
				retval = -ENOMEM;
			else {
				action->handler = handler;
				action->flags   = irqflags;
				action->mask    = 0;
				action->name    = devname;
				action->next    = NULL;
				action->dev_id  = dev_id;
				ucb	     = (unitblk_t *) dev_id;
				ucb->unitaction = (void *) action;

				p = irq_action + irq - 1;
				if ((old = *p) != NULL) {
					/*-----------------------------------------*/
					/* add new interrupt at end of irq queue   */
					/*---------------------------------------- */ 
					do {
						p = &old->next;
						old = *p;
					} while (old);
				}
				if (action->flags & SA_SAMPLE_RANDOM)
					rand_initialize_irq(irq);
			}
		}
	}

	return retval;
}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - free_irq.				   */
/*							  */
/* Function   - Unregister our interest in a given interrupt*/
/*	      subclass for a given device.		*/
/*							  */
/************************************************************/

void
free_irq(unsigned int irq, void *dev_id)
{
	struct irqaction * action, **p;
	unsigned long flags;
	unitblk_t *ucb;

	if (irq > NR_IRQS) {
		printk("Trying to free IRQ%d\n",irq);
		return;
	}
	for (p = irq + irq_action - 1;
	     (action = *p) != NULL;
	     p = &action->next) {
		if (action->dev_id != dev_id)
			continue;

		/* Found it - now free it */
		*p  = action->next;
		ucb = (unitblk_t *) dev_id;
		ucb->unitaction = NULL;
		kfree(action);
		return;
	}
}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - probe_irq_on.			       */
/*							  */
/* Function   - Determine what interrupt subclasses are     */
/*	      enabled.				    */
/*							  */
/************************************************************/

unsigned long
probe_irq_on (void)
{
	cr0_t cr0;
	unsigned char mask;
	long result;

	cr0.raw = _stctl_r0();
	result  = cr0.bits.iksm;

	return result;
}

/*===================== End of Function ====================*/

/************************************************************/
/*							  */
/* Name       - probe_irq_off.			      */
/*							  */
/* Function   - Determine what interrupt subclasses are     */
/*	      disabled.				   */
/*							  */
/************************************************************/

int
probe_irq_off (unsigned long irqs)
{
	cr0_t cr0;
	unsigned char mask;
	long result;

	mask    = ~(1 << (irqs & 7));
	cr0.raw = _stctl_r0();
	result  = cr0.bits.iksm & mask;

	return result;
}

/*===================== End of Function ====================*/
