/*
 *  linux/arch/i370/kernel/traps.c
 *  copied from the ppc implementation; ppc copyrights apply to this code.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

/*
 * This file handles the architecture-dependent parts of hardware exceptions
 */


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/asm.h>
#include <asm/param.h>
#include <asm/ptrace.h>
#include <asm/signal.h>
#include <asm/system.h>

#ifdef CONFIG_XMON
extern void xmon(struct pt_regs *regs);
extern int xmon_bpt(struct pt_regs *regs);
extern int xmon_sstep(struct pt_regs *regs);
extern int xmon_iabr_match(struct pt_regs *regs);
extern int xmon_dabr_match(struct pt_regs *regs);
extern void (*xmon_fault_handler)(struct pt_regs *regs);
#endif

#ifdef CONFIG_XMON
void (*debugger)(struct pt_regs *regs) = xmon;
int (*debugger_bpt)(struct pt_regs *regs) = xmon_bpt;
int (*debugger_sstep)(struct pt_regs *regs) = xmon_sstep;
int (*debugger_iabr_match)(struct pt_regs *regs) = xmon_iabr_match;
int (*debugger_dabr_match)(struct pt_regs *regs) = xmon_dabr_match;
void (*debugger_fault_handler)(struct pt_regs *regs);
#else
#ifdef CONFIG_KGDB
void (*debugger)(struct pt_regs *regs);
int (*debugger_bpt)(struct pt_regs *regs);
int (*debugger_sstep)(struct pt_regs *regs);
int (*debugger_iabr_match)(struct pt_regs *regs);
int (*debugger_dabr_match)(struct pt_regs *regs);
void (*debugger_fault_handler)(struct pt_regs *regs);
#endif
#endif

void instruction_dump (unsigned short *pc)
{
   int i;

   printk("Instruction DUMP:");
   for(i = -6; i < 12; i++)
      printk("%c%04x%c",i?' ':'<',pc[i],i?' ':'>');
   printk("\n");
}


/*
 * Trap & Exception support
 */

void
_exception(int signr, struct pt_regs *regs)
{
   if (!user_mode(regs))
   {
      show_regs(regs);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
      debugger(regs);
#endif
      // print_backtrace((unsigned long *)regs->gpr[0]);
      instruction_dump((unsigned short *)instruction_pointer(regs));
      panic("Exception in kernel psw.addr=%lx signal %d",regs->psw.addr,signr);
   }
   force_sig(signr, current);
}

psw_t
MachineCheckException(struct pt_regs *regs)
{
   psw_t retval;
   if ( !user_mode(regs) )
	{
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		if (debugger_fault_handler) {
			debugger_fault_handler(regs);
			return;
		}
#endif
		printk("Machine check in kernel mode.\n");
		printk("regs %p ",regs);
		show_regs(regs);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		debugger(regs);
#endif
		// print_backtrace((unsigned long *)regs->gpr[1]);
		instruction_dump((unsigned long *)regs->psw.addr);
		panic("machine check");
	}
	_exception(SIGSEGV, regs);	
   return retval;
}


psw_t
ProgramCheckException(struct pt_regs *regs)
{
   psw_t retval;
#ifdef JUNK_XXX
	if (regs->msr & 0x100000) {
		/* IEEE FP exception */
		_exception(SIGFPE, regs);
	} else if (regs->msr & 0x20000) {
		/* trap exception */
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		if (debugger_bpt(regs))
			return;
#endif
		_exception(SIGTRAP, regs);
	} else {
		_exception(SIGILL, regs);
	}
#endif 
   return retval;
}

extern inline void i370_halt (void) 
{
	psw_t halt_psw;
	halt_psw.flags = HALT_PSW;
	halt_psw.addr = 0xffffffff;
	_lpsw (*((unsigned long long *) &halt_psw));
}
	
void
RestartException(i370_interrupt_state_t *saved_regs)
{
	printk ("restart exception\n");
	panic("machine check");
}

void
InputOutputException(i370_interrupt_state_t *saved_regs)
{
	printk ("io exception\n");
}

void
ExternalException (i370_interrupt_state_t *saved_regs)
{
	unsigned short code;
	unsigned long long ticko;

	/* get the interruption code */
	code = *((unsigned short *) EXT_INT_CODE);

	/* currently we only handle and expect clock interrupts */
	if ( EI_CLOCK_COMP != code) {
		printk ("unexpected external exception code=0x%x\n", code);
		panic ("unexpected external exception\n");
	}

	/* get and set the new clock value */
	/* clock ticks every 250 picoseconds (actually, every
	 * microsecond/4K). We want an interrupt every HZ of 
	 * a second */
	ticko = _stckc ();
	ticko += (1000000/HZ) << 12;
	_sckc (ticko);
	
	/* let Linux do its timer thing */
	do_timer (saved_regs);
}

/* ================================================================ */
/* do SLIH intrerrupt handling (the bottom half) */
/* pseudocode:  */

#ifdef LATER

void ret_from_syscall (void) 
{
	int do_it_again = 1;

	while (do_it_again) {
		cli();
		do_it_again = 0:

		/* bitwise and */
		if (bh_mask & bh_active) {
			do_bottom_half ();  // handle_bottom_half()
			do_it_again = 1;
		}
		if (return_to_user) {
			if (need_reschedule) {
				schedlue ();
				do_it_again = 1;
				continue;
			}
			if (sig_pending) {
				do_signal ();
				do_it_again = 1;
				continue;
			}
		}
		cli ();
	}
}
#endif

/* ================================================================ */


void
StackOverflow(struct pt_regs *regs)
{
	// printk(KERN_CRIT "Kernel stack overflow in process %p, sp=%lx\n",
	//        current, regs->gpr[1]);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	debugger(regs);
#endif
	show_regs(regs);
	// print_backtrace((unsigned long *)regs->gpr[1]);
	// instruction_dump((unsigned long *)regs->psw.addr);
	panic("kernel stack overflow");
}

/* ================================================================ */
/* wire in the interrupt vectors */

extern void SupervisorCall (void);
extern void External (void);

/*
 * we assume that we initialize traps while in real mode, i.e.
 * i.e. that the fixed storage locations (fsl) at low memory 
 * are where they're supposed to be.
 */

__initfunc(void trap_init(void))
{
	unsigned long long clock_reset;
	unsigned long *sz;
	psw_t psw;
	printk ("trap init");

	/* clear any pending timer interrupts before installing
	 * external interrupt handler. Do this by setting the
         * clock comparator to -1.
	 */
	clock_reset = 0xffffffffffffffffLL;
	_sckc (clock_reset);

	/* store the offset between the task struct and the kernel stack
	 * pointer in low memory where we can get at it for calculation
	 */
	sz = (unsigned long *) INTERRUPT_BASE;
	*(sz-1) = (unsigned long) &(((struct task_struct *) 0) ->tss.ksp);
	*(sz-2) = (unsigned long) &(((struct task_struct *) 0) ->tss.regs);
	*(sz-3) = (unsigned long) &(((struct task_struct *) 0) ->tss.tca[0]);

	// install the SVC handler
	psw.flags = PSW_VALID;        // disable all interupts
	psw.addr = ((unsigned long) SupervisorCall) | (1<<31); 
	*((psw_t *) SVC_PSW_NEW) = psw;

	// install the External Interrupt (clock) handler
	psw.flags = PSW_VALID;        // disable all interupts
	psw.addr = ((unsigned long) External) | (1<<31); 
	*((psw_t *) EXTERN_PSW_NEW) = psw;
}

/* ===================== END OF FILE =================================== */
