/*
 * linux/arch/i370/kernel/process.c
 *
 * Derived from "linux/arch/ppc/kernel/process.c", copyrights apply:
 *   Copyright (C) 1995  Linus Torvalds
 *   Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org
 *   Copyright (C) Cort Dougan (cort@cs.nmt.edu)
 *   Copyright (C) Paul Mackerras (paulus@cs.anu.edu.au)
 *
 * Modified to implement Linux forr the ESA/390 class mainframes
 *   Copyright (C) 1999 Linas Vepstas (linas@linas.org)
 *
 * XXX this minimally works in a broken-like way for the ESA/390
 * needs a lot of fixin to be really operational...
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/asm.h>
#include <asm/elf.h>
#include <asm/pgtable.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>

#define __KERNEL_SYSCALLS__ 1
#include <asm/unistd.h>

static struct vm_area_struct init_mmap = INIT_MMAP;
static struct fs_struct init_fs = INIT_FS;
static struct file * init_fd_array[NR_OPEN] = { NULL, };
static struct files_struct init_files = INIT_FILES;
static struct signal_struct init_signals = INIT_SIGNALS;

struct mm_struct init_mm = INIT_MM;
union task_union init_task_union = { INIT_TASK };

/* only used to get secondary processor up */
struct task_struct *current_set[NR_CPUS] = {&init_task, };

struct task_struct *current = &init_task;

/* init_ksp is used only in head.S during startup to set up the inital stack */
const unsigned long init_ksp __initdata = (unsigned long) init_stack;

/* =================================================================== */

static inline unsigned long
kernel_stack_top(struct task_struct *tsk)
{
	unsigned long top = ((unsigned long)tsk) + 8192 - 160;
	top = ((top+7) >> 3) << 3;  /* double-word align */
        return top;
}

static inline unsigned long
task_top(struct task_struct *tsk)
{
        return ((unsigned long)tsk) + sizeof(struct task_struct);
}

/* =================================================================== */

int dump_fpu(struct pt_regs *regs, elf_fpregset_t *fpregs) { 
	printk ("dump fpu \n");
	return 0;
}

void 
show_regs(struct pt_regs * regs)
{
	printk("PSW flags: %08lX PSW addr: %08lX \n",
		regs->psw.flags, regs->psw.addr);
	printk (" cr0: %08lX  cr1: %08lX \n", regs->cr0.raw, regs->cr1.raw);

        /* Note: if debugging disabled, r5-r10 may not be valid */
        printk ("  r0: %08lX   r1: %08lX   r2: %08lX   r3: %08lX \n", 
		regs->irregs.r0, regs->irregs.r1, regs->irregs.r2, regs->irregs.r3);

        printk ("  r4: %08lX   r5: %08lX   r6: %08lX   r7: %08lX \n", 
		regs->irregs.r4, regs->irregs.r5, regs->irregs.r6, regs->irregs.r7);

        printk ("  r8: %08lX   r9: %08lX  r10: %08lX  r11: %08lX \n", 
		regs->irregs.r8, regs->irregs.r9, regs->irregs.r10, regs->irregs.r11);

        printk (" r12: %08lX  r13: %08lX  r14: %08lX  r15: %08lX \n", 
		regs->irregs.r12, regs->irregs.r13, regs->irregs.r14, regs->irregs.r15);

}

void
print_backtrace (unsigned long stackp)
{
        int cnt = 0;
	i370_elf_stack_t *sp;

        printk("Call Backtrace:\n");

	do 
	{
		/* If the stack is at a very high address, assume 
		 * that its a user-space stack pointer and that 
		 * it needs address trnslation. 
		 */
		if (0x7f000000 < stackp) {
			pte_t *pte = find_pte (current->mm, stackp);
			if (!pte || pte_none(*pte)) {
				make_pages_present (stackp, 0x7fffffff);
				pte = find_pte (current->mm, stackp);
			}
			stackp = pte_page (*pte) | (stackp & ~PAGE_MASK);
		}
		sp = (i370_elf_stack_t *) stackp;

		printk ("   %02d   base=0x%lx link=0x%lx stack=%p\n", 
			cnt, sp->caller_r3, sp->caller_r14, stackp);
		stackp = sp->caller_sp;
		cnt ++;
        } while (stackp &&  (cnt < 6)) ;
        printk("\n");
}


/* =================================================================== */

#define CHECK_STACK
#ifdef CHECK_STACK

/* check to make sure the kernel stack is healthy */
/* check_stack copied raw from from the ppc implementation */
int check_stack(struct task_struct *tsk)
{
	unsigned long stack_top = kernel_stack_top(tsk);
	unsigned long tsk_top = task_top(tsk);
	int ret = 0;

	if (!tsk)
	{
		printk ("check_stack(): bad task %p\n",tsk);
		return 1;
	}
	
	/* check if stored ksp is bad */
	if ( (tsk->tss.ksp > stack_top) || (tsk->tss.ksp < tsk_top) )
	{
		printk("stack out of bounds: %s/%d\n"
		       " tsk_top %08lx ksp %08lx stack_top %08lx\n",
		       tsk->comm,tsk->pid,
		       tsk_top, tsk->tss.ksp, stack_top);
		ret |= 2;
	}
	
	/* check if stack ptr RIGHT NOW is bad */
	if ( (tsk == current) && ((_get_SP() > stack_top ) || (_get_SP() < tsk_top)) )
	{
		printk("current stack ptr out of bounds: %s/%d\n"
		       " tsk_top %08lx sp %08lx stack_top %08lx\n",
		       current->comm,current->pid,
		       tsk_top, _get_SP(), stack_top);
		ret |= 4;
	}

	/* check amount of free stack */
	if ( 900 > (tsk->tss.ksp - tsk_top) )
	{
		printk("low on stack space: %s/%d\n"
		       " tsk_top %08lx ksp %08lx stack_top %08lx\n",
		       tsk->comm,tsk->pid,
		       tsk_top, tsk->tss.ksp, stack_top);
		ret |= 8;
	}
	
	if (ret)
	{
		panic("bad kernel stack");
	}
	return(ret);
}
#endif /* CHECK_STACK */

/* =================================================================== */

/*
 * Initialize a thread for running a user-land program
 * This is where:
 * -- we enable DAT (address translation)
 * -- we set the problem state bit in the PSW
 * -- set the user's stack pointer
 * -- set the address at which to start executing the user process
 */
void 
i370_start_thread(struct pt_regs *regs, unsigned long nip, unsigned long sp)
{
        cr0_t cr0;

	printk ("i370_start_thread(): setup for user-space thread\n");
        set_fs(USER_DS);
       	regs->psw.flags &= (PSW_SPACE_MASK | PSW_WAIT);
        regs->psw.flags |= USER_PSW;
        regs->psw.addr = nip | PSW_31BIT;
	/* Currently, the elf stack grows downwards & we need to make
	   some room for the libc _start() routine to pas parameters to main.  
           sizeof stackframe == 144 bytes, args = 16B for total of 0xa0 = 160
           regs->irregs.r13 = sp - sizeof (elf_stack_t) - 16 ; */
        regs->irregs.r13 = sp - 0xa0;
        regs->irregs.r15 = nip | PSW_31BIT;

	/* boffo ace rimer in other regs */
	regs->irregs.r0 = 0xaceb0ff0;
	regs->irregs.r1 = 0xaceb0ff0;
	regs->irregs.r2 = 0xaceb0ff0;
	regs->irregs.r3 = 0xaceb0ff0;
	regs->irregs.r4 = 0xaceb0ff0;
	regs->irregs.r5 = 0xaceb0ff0;
	regs->irregs.r6 = 0xaceb0ff0;
	regs->irregs.r7 = 0xaceb0ff0;
	regs->irregs.r8 = 0xaceb0ff0;
	regs->irregs.r9 = 0xaceb0ff0;
	regs->irregs.r10 = 0xaceb0ff0;
	regs->irregs.r11 = 0xaceb0ff0;
	regs->irregs.r12 = 0xaceb0ff0;
	regs->irregs.r14 = 0xaceb0ff0;

        cr0.raw = _stctl_r0();
        cr0.bits.tf = 0x16;
        _lctl_r0(cr0.raw);
}

/* =================================================================== */

// switch_to does the task switching.  
// I'm not to clear on what it needs to do ... 
// ... needs to switch the stack pointer, at least ... 
//     if we just switch the stack pointer, then I don't think we need to
//     save/restore any registers, since merely returning from this routine
//     will accomplish all the sve/restore we need.
// ... change addressing modes (potentially, from real 
//     to primary or v.v.  ... then again, we can defer changing addressing
//     until we return to the user-level process. Ditto for the next two
//     steps ...
// ... modify the segment table CR1 and mabye CR3 key  ...
// ... Purge TLB ...
//
// NB the first time through, switch_to() actually returns as if
// it were do_fork() to anyone who had called do_fork(), (even
// though switch_to() was called in schedule()).  This is because when 
// the stack unwinds during the subr return, it unwinds into a copy 
// of it's parent's stack (minux one stackframe).  And the last thing 
// the parent had done was a fork, ergo, that's were we return to.
// 
// A non-zero return value indicates an error.

int
switch_to(struct task_struct *prev, struct task_struct *new)
{
	struct thread_struct *new_tss, *old_tss;

        unsigned long s = __cli ();

#ifdef CHECK_STACK
        check_stack(prev);
        check_stack(new);
#endif /* CHECK_STACK */

#define SHOW_TASK_SWITCHES
#ifdef SHOW_TASK_SWITCHES
	printk("task switch ");
        printk("%s/%d -> %s/%d PSW 0x%lx 0x%lx cpu %d root %p/%p\n",
               prev->comm,prev->pid,
               new->comm,new->pid,
               new->tss.regs->psw.flags,
               new->tss.regs->psw.addr,
               new->processor,
               new->fs->root,prev->fs->root);
#endif
#ifdef __SMP__
        prev->last_processor = prev->processor;
        current_set[smp_processor_id()] = new;
#endif /* __SMP__ */

        old_tss = &current->tss;
        new_tss = &new->tss;

	/* save and restore flt point regs.  We do this here because 
	 * it is NOT done at return from interrupt (in order to save
	 * some overhead) */
	_store_fpregs (old_tss->fpr);
	_load_fpregs (new_tss->fpr);

	current = new;

	/* switch control registers */
	/* cr1 contains the segment table origin */
	_lctl_r1 (new_tss->regs->cr1.raw);

	/* switch kernel stack pointers */
	old_tss->ksp = _get_SP();
	_set_SP (new_tss->ksp);

	/* purge TLB (XXX is this really needed here ???) */
	_ptlb();

	/* note "s" is from a different stack ...  */
        __restore_flags (s);

	/* return as if from do_fork() */
	return 0;
}


/* =================================================================== */

void exit_thread(void)
{
	printk ("exit_thread(): not implemented \n");
}

void flush_thread(void)
{
	printk ("flush_thread(): not implemented \n");
}

void
release_thread(struct task_struct *t)
{
	printk ("release_thread(): not implemented \n");
	i370_halt();
}

void 
i370_sys_exit (void) 
{
	printk ("i370_sys_exit(): not implemented \n");
	i370_halt();
}

/* =================================================================== */

/*
 * Copy a thread..
 * incomplete and buggy, sorta works ....  give it a shot
 * What this function does/needs to do:
 * -- Copy architecture-dependent parts of the task structure.
 * -- Set up the user stack pointer
 * -- Set up page tables ??? !!
 * -- Load CR1 with page table origin ??!!
 * -- Copy PSW flags (in particular, the DAT flag) ???
 *
 * argument p is the copy_to_proc;  copy_from is "current"
 * usp and regs are exactly what was passed to do_fork.
 * It is assumed that usp is the stack pointer before
 * the svc took place.
 *
 * Return 0 if success, non-zero if not
 *
 * XXX we gotta copy the user-space thread, as wel ... 
 *
 * XXX we need to check to see if usp is indeed a user-space
 * stack pointer, and if so, set irregs.r13 to it.  We do not
 * need to actually coppy the user-land stack, that happens
 * "automagically".
 *
 * XXX no need to copy page tables ??? I think this happens automagically
 */

int
copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
            struct task_struct * p, struct pt_regs * regs)
{
	unsigned long srctop, dsttop;
	i370_elf_stack_t *srcsp, *dstsp, *this_frame;
	i370_elf_stack_t *lastsrc, *lastdst;
	void *srcbase, *dstbase;
	unsigned long delta;
	i370_interrupt_state_t *srcregs, **dstregs;

	printk("i370_copy_thread, usp=0x%lx\n", usp);

	/* Copy the kernel stack, and fix up the entries in it.
	 * Don't copy the current frame: we want the new stack
	 * to unwinds is if it were do_fork(), and not to do_fork().
	 */
	srctop = kernel_stack_top (current);
	dsttop = kernel_stack_top (p);

	this_frame = (i370_elf_stack_t *) _get_SP();
        this_frame = (i370_elf_stack_t *) (this_frame->caller_sp);
	srcsp = this_frame;

	delta = srctop - dsttop;

	srcbase = (void *) task_top (current);
	dstbase = (void *) task_top (p);
	memcpy (dstbase, srcbase, 8192-sizeof(struct task_struct));
	dstsp = (i370_elf_stack_t *) (((unsigned long) srcsp) - delta);
	do {
		dstsp -> caller_sp = srcsp->caller_sp - delta;
		dstsp -> callee_sp = srcsp->callee_sp - delta;
		lastsrc = srcsp;
		lastdst = dstsp;
		srcsp = (i370_elf_stack_t *) (srcsp -> caller_sp);
		dstsp = (i370_elf_stack_t *) (dstsp -> caller_sp);
	} while (srcsp);
	lastdst -> caller_sp = 0;

	/* Switch_to grabs the current ksp out of tss.ksp */
	p->tss.ksp = ((unsigned long) this_frame) - delta;

	/* Interrupt regs are stored on stack as well */
	srcregs = current->tss.regs;
	dstregs = &(p->tss.regs);
	do {
		*dstregs = (i370_interrupt_state_t *) 
			(((unsigned long) srcregs) - delta);
		(*dstregs)->oldregs =  (i370_interrupt_state_t *)
			(((unsigned long) (srcregs->oldregs)));     
		srcregs = srcregs->oldregs;
		dstregs = &((*dstregs)->oldregs);
	} while (srcregs);

	/* Now copy the 'far side' of the stack, the part that lies
	 * on the other side of the SVC that go us here.  There are
	 * two possibilities here: that we came from user-land,
	 * or that we came from the kernel. If we came from the 
	 * kernel, then in fact we are on the same stack frame.  
	 */
	if (regs->psw.flags & PSW_PROB) {
		printk("i370_copy_thread, copy of user stack not yet supported\n");
		i370_halt();
	} else {
		/* validate expected location of the stack */
		if ((usp < (unsigned long) lastsrc) || (usp > srctop)) {
			printk("i370_copy_thread, bad usp=0x%lx, "
				"lastsrc=%p, stcktop=0x%lx \n",
				usp, lastsrc, srctop);
			i370_halt();
		}

		/* Same kernel stack, just fix up entries. */
		srcsp = (i370_elf_stack_t *) usp;
		dstsp = (i370_elf_stack_t *) (usp - delta);
		do {
			dstsp -> caller_sp = srcsp->caller_sp - delta;
			dstsp -> callee_sp = srcsp->callee_sp - delta;
			lastdst = dstsp;
			srcsp = (i370_elf_stack_t *) (srcsp -> caller_sp);
			dstsp = (i370_elf_stack_t *) (dstsp -> caller_sp);
		} while (srcsp);
		lastdst -> caller_sp = 0;
	}

#ifdef __SMP__
        if ( (p->pid != 0) || !(clone_flags & CLONE_PID) )
                p->tss.smp_fork_ret = 1;
        p->last_processor = NO_PROC_ID;
#endif /* __SMP__ */

	return 0;
}

/* =================================================================== */

/* 
 * note:
 * do_fork will call copy_thread, passing sp and regs to it
 */

asmlinkage int 
i370_sys_fork (void) 
{
	int res = 0;
	printk ("i370_sys_fork(): not implemented \n");

	lock_kernel();
	// XXX whatever
	// res = do_fork(SIGCHLD, regs->gpr[1], regs);
	i370_halt();

	/* only parent returns here */
#ifdef __SMP__
	/* When we clone the idle task we keep the same pid but
	 * the return value of 0 for both causes problems.
	 * -- Cort
	 */
	if ((current->pid == 0) && (current == &init_task))
		res = 1;
#endif /* __SMP__ */
	unlock_kernel();
	return res;
}

/* =================================================================== */

asmlinkage int 
i370_sys_clone (unsigned long clone_flags)
{
	struct pt_regs *regs;
        int res;

	printk ("i370_sys_clone flags=0x%x\n", clone_flags);
        lock_kernel();
	regs = current->tss.regs;

	/* all args pass to copy_thread */
        res = do_fork(clone_flags, regs->irregs.r13, regs);

#ifdef __SMP__
        /* When we clone the idle task we keep the same pid but
         * the return value of 0 for both causes problems.
         * -- Cort
         */
        if ((current->pid == 0) && (current == &init_task))
                res = 1;
#endif /* __SMP__ */
        unlock_kernel();

	printk ("i370_sys_clone(): after do_fork, res=%d\n", res);

        return res;
}

/* =================================================================== */
/*
 * i370_kernel_thread ... creates a new kernel thread.
 * Called during initialization, and during module load to create
 * misc important kernel threads. 
 *
 * The child is supposed to then call the arg fn(), and never return.
 * If for any reason fn() returns, the child thread is supposed to be 
 * cleaned up and discarded. 
 * 
 * clone() is a system call, it results in i370_sys_clone() being
 * called.  In turn, i370_sys_clone just calls do_fork() which then 
 * calls copy_thread().  We can't call do_fork directly, we must 
 * svc into it in order to be able to schedule and return properly 
 * in the child process.
 */

long 
i370_kernel_thread(unsigned long flags, int (*fn)(void *), void *args)  
{
	long pid;
	printk ("i370_kernel_thread\n");
	pid = clone (flags);
	printk ("i370_kernel_thread(): return from clone, pid=%ld\n",pid);
        if (pid) return pid;
	fn (args);
	while (1) {_exit (1); } 
	return 0;
}

/* ========================== END OF FILE =========================== */
