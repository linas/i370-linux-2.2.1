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

extern void sys_call_table(void);
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
extern inline void i370_halt (void) 
{
	psw_t halt_psw;
	halt_psw.flags = HALT_PSW;   /* load disabled wait state */
	halt_psw.addr = 0xffffffff;
	_lpsw (*((unsigned long long *) &halt_psw));
}
	

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
		panic ("Exception in kernel psw.addr=%lx signal %d",
			regs->psw.addr,signr);
	}
	force_sig(signr, current);
}

void
MachineCheckException(i370_interrupt_state_t *saved_regs)
{
	printk ("machine check \n");
	i370_halt();

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
}


void
ProgramCheckException(i370_interrupt_state_t *saved_regs)
{
	long pfx_prg_code = *((long *) PFX_PRG_CODE);
	long pfx_prg_trans = *((long *) PFX_PRG_TRANS);

#ifdef JUNK_XXX_RIMPLEMENT_THIS_SOMEDAY
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
	printk ("Program Check code=0x%x trans=0x%x\n", 
		pfx_prg_code, pfx_prg_trans);

	switch (pfx_prg_code) 
	{
		case PIC_PRIVLEDGED:
			printf ("privleded \n");
			break;
		casse PIC_ADDRESSING:
			printf ("addressing\n");
			break;
		default:
			printk ("unexpected prgram check code\n");
	}

	i370_halt(); 
}

void
RestartException(i370_interrupt_state_t *saved_regs)
{
	printk ("restart exception\n");
	panic("restart");
	i370_halt();
}

void
InputOutputException(i370_interrupt_state_t *saved_regs)
{
	long pfx_io_parm = *((long *) PFX_IO_PARM);
	long pfx_subssy_id = *((long *) PFX_SUBSYS_ID);

	//printk ("i/o interrupt subchannel=0x%x param=0x%x\n", 
	//	pfx_subsysid, pfx_io_parm);
}

void
ExternalException (i370_interrupt_state_t *saved_regs)
{
	unsigned short code;
	unsigned long long ticko;

	/* get the interruption code */
	code = *((unsigned short *) PFX_EXT_CODE);

	/* currently we only handle and expect clock interrupts */
	if ( EI_CLOCK_COMP != code) {
		printk ("unexpected external exception code=0x%x\n", code);
		panic ("unexpected external exception\n");
		i370_halt();
	}

	/* get and set the new clock value */
	/* clock ticks every 250 picoseconds (actually, every
	 * microsecond/4K). We want an interrupt every HZ of 
	 * a second */
	/* XXX it is more correct to use stckc but on a really slow 
         * VM day VM can be so overburdened that we never catch up... */
	/* ticko = _stckc (); */
	ticko = _stck ();
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
extern void InputOutput (void);
extern void ProgramCheck (void);

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

	// install the I/O Interrupt handler
	psw.flags = PSW_VALID;        // disable all interupts
	psw.addr = ((unsigned long) InputOutput) | (1<<31); 
	*((psw_t *) IO_PSW_NEW) = psw;

	// restart quick hack
	psw.flags = 0x000a0000;
	psw.addr = 0x0000ffff;
	*((psw_t *) IPL_PSW_NEW) = psw;

	// install the ProgramCheck handler
	psw.flags = PSW_VALID;        // disable all interupts
	psw.addr = ((unsigned long) ProgramCheck) | (1<<31); 
	*((psw_t *) PROG_PSW_NEW) = psw;
}

/* ===================== END OF FILE =================================== */
