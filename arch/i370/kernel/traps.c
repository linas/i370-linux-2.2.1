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

void instruction_dump (unsigned long *pc)
{
        int i;

        if((((unsigned long) pc) & 3))
                return;

        printk("Instruction DUMP:");
        for(i = -3; i < 6; i++)
                printk("%c%08lx%c",i?' ':'<',pc[i],i?' ':'>');
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
		print_backtrace((unsigned long *)regs->gpr[1]);
		instruction_dump((unsigned long *)instruction_pointer(regs));
		panic("Exception in kernel ia %lx signal %d",regs->psw_ia,signr);
	}
	force_sig(signr, current);
}

void
MachineCheckException(struct pt_regs *regs)
{
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
		print_backtrace((unsigned long *)regs->gpr[1]);
		instruction_dump((unsigned long *)regs->psw_ia);
		panic("machine check");
	}
	_exception(SIGSEGV, regs);	
}

void
UnknownException(struct pt_regs *regs)
{
	printk("Bad trap at IA: %lx, PSW: %lx\n",
	       regs->psw_ia, regs->psw_bits);
	_exception(SIGTRAP, regs);	
}

void
InstructionBreakpoint(struct pt_regs *regs)
{
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	if (debugger_iabr_match(regs))
		return;
#endif
	_exception(SIGTRAP, regs);
}

void
ProgramCheckException(struct pt_regs *regs)
{
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
}

void
RestartException(struct pt_regs *regs)
{
}

void
SupervisorCallException(struct pt_regs *regs)
{
}

void
InputOutputException(struct pt_regs *regs)
{
}

void
ExternalException(struct pt_regs *regs)
{
}

void
SingleStepException(struct pt_regs *regs)
{
	// regs->msr &= ~MSR_SE;  /* Turn off 'trace' bit */
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	if (debugger_sstep(regs))
		return;
#endif
	_exception(SIGTRAP, regs);	
}


void
StackOverflow(struct pt_regs *regs)
{
	printk(KERN_CRIT "Kernel stack overflow in process %p, r1=%lx\n",
	       current, regs->gpr[1]);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	debugger(regs);
#endif
	show_regs(regs);
	print_backtrace((unsigned long *)regs->gpr[1]);
	instruction_dump((unsigned long *)regs->psw_ia);
	panic("kernel stack overflow");
}

void
trace_syscall(struct pt_regs *regs)
{
	printk("Task: %p(%d), psw: %08lX/%08lX, Syscall: %3ld, Result: %ld\n",
	       current, current->pid, regs->psw_bits, regs->psw_ia, regs->gpr[0],
	       regs->gpr[3]);
}

__initfunc(void trap_init(void))
{
   printk ("trap init");
}
