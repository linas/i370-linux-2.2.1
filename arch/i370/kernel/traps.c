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
		panic("Exception in kernel ia %lx signal %d",regs->psw.addr,signr);
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
		print_backtrace((unsigned long *)regs->gpr[1]);
		instruction_dump((unsigned long *)regs->psw.addr);
		panic("machine check");
	}
	_exception(SIGSEGV, regs);	
   return retval;
}

psw_t
UnknownException(struct pt_regs *regs)
{
   psw_t retval, *fsl;
	printk("Bad trap at IA: %lx, PSW: %lx\n",
	       regs->psw.addr, regs->psw.flags);
	_exception(SIGTRAP, regs);	
   return retval;
}

psw_t
InstructionBreakpoint(struct pt_regs *regs)
{
   psw_t retval, *fsl;
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	if (debugger_iabr_match(regs))
		return;
#endif
	_exception(SIGTRAP, regs);
   return retval;
}

psw_t
ProgramCheckException(struct pt_regs *regs)
{
   psw_t retval, *fsl;
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

psw_t
RestartException(struct pt_regs *regs)
{
   psw_t retval, *fsl;
   printk ("restart exception\n");
   panic("machine check");
   fsl = (psw_t *) IPL_PSW_OLD; 
   retval = *fsl; 
   return retval;
}

psw_t
SupervisorCallException(struct pt_regs *regs)
{
   psw_t retval, *fsl;

   printk ("svc exception\n");
   fsl = (psw_t *) SVC_PSW_OLD; 
   retval = *fsl; 
   return retval;
}

psw_t
InputOutputException(struct pt_regs *regs)
{
   psw_t retval, *fsl;
   printk ("io exception\n");
   fsl = (psw_t *) IO_PSW_OLD; 
   retval = *fsl; 
   return retval;
}

psw_t
ExternalException(struct pt_regs *regs)
{
   psw_t retval, *fsl;
   printk ("external exception\n");
   fsl = (psw_t *) EXTERN_PSW_OLD; 
   retval = *fsl; 
   return retval;
}

psw_t
SingleStepException(struct pt_regs *regs)
{
   psw_t retval;
	// regs->msr &= ~MSR_SE;  /* Turn off 'trace' bit */
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	if (debugger_sstep(regs))
		return;
#endif
	_exception(SIGTRAP, regs);	
   return retval;
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
	instruction_dump((unsigned long *)regs->psw.addr);
	panic("kernel stack overflow");
}

void
trace_syscall(struct pt_regs *regs)
{
	printk("Task: %p(%d), psw: %08lX/%08lX, Syscall: %3ld, Result: %ld\n",
	       current, current->pid, regs->psw.flags, regs->psw.addr, regs->gpr[0],
	       regs->gpr[3]);
}

extern void * SupervisorCall;

/*
 * we assume that we initialize traps while in real mode, i.e.
 * i.e. that the fixed storage locations (fsl) at low memory 
 * are hwere they're supposed to be.
 */

__initfunc(void trap_init(void))
{
   psw_t psw, *fsl;
   printk ("trap init");

   fsl = IPL_PSW_NEW;

   fsl = (psw_t *) SVC_PSW_NEW;
   psw.flags = PSW_IO | PSW_EXTERN | PSW_MACH;     // disable interupts
   psw.addr = SupervisorCall || (1<<31); 
   *fsl = psw;
}
