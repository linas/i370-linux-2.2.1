
/*
 * mostly copied from the ppc implementation
 * XXX this is fairly broken for the 370
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/asm.h>
#include <asm/elf.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>

static struct vm_area_struct init_mmap = INIT_MMAP;
static struct fs_struct init_fs = INIT_FS;
static struct file * init_fd_array[NR_OPEN] = { NULL, };
static struct files_struct init_files = INIT_FILES;
static struct signal_struct init_signals = INIT_SIGNALS;

struct mm_struct init_mm = INIT_MM;
union task_union init_task_union = { INIT_TASK };

/* only used to get secondary processor up */
// struct task_struct *current_set[NR_CPUS] = {&init_task, };

struct task_struct *current = &init_task;

/* init_ksp and init_tca are used only in head.S during startup 
 * to set up the inital stack */
const unsigned long init_ksp __initdata = init_stack;
const unsigned long init_kstend __initdata = (unsigned long) (((char *) &init_task)+4096);
const unsigned long init_tca __initdata = (unsigned long) (&init_task.tss.tca[0]);

unsigned long
kernel_stack_top(struct task_struct *tsk)
{
        return ((unsigned long)tsk) + sizeof(union task_union);
}

unsigned long
task_top(struct task_struct *tsk)
{
        return ((unsigned long)tsk) + sizeof(struct task_struct);
}

void show_regs(struct pt_regs * regs)
{
   // int i;

   //for (i=0; i<16; i+=2) {
   //   printk ("GPR %d: %08lX   GPR %d %08lX \n", 
   //      i, regs->gpr[i], i+1, regs->gpr[i+1]);
   //}
   printk("PSW flags: %08lX PSW addr: %08lX \n",
     regs->psw.flags, regs->psw.addr);

}

void
print_backtrace(unsigned long *sp)
{
//        int cnt = 0;
//        unsigned long i;

        printk("Call Backtrace:\n");
#ifdef BOGUS_XXX
        while (sp) {
                if (__get_user( i, &sp[1] ))
                        break;
                if (cnt++ % 7 == 0)
                        printk("\n");
                printk("%08lX ", i);
                if (cnt > 32) break;
                if (__get_user(sp, (unsigned long **)sp))
                        break;
        }
#endif
        printk("\n");
}


#define CHECK_STACK
#ifdef CHECK_STACK

/* check to make sure the kernel stack is healthy */
/* check_stack copied raw from from the ppc implementation */
int check_stack(struct task_struct *tsk)
{
	unsigned long stack_top = kernel_stack_top(tsk);
	unsigned long tsk_top = task_top(tsk);
	int ret = 0;
	unsigned long *i=0;

	if ( !tsk )
		printk("check_stack(): tsk bad tsk %p\n",tsk);
	
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

#if 1	
	/* check amount of free stack */
	for ( i = (unsigned long *)task_top(tsk) ; 
		i < (unsigned long * ) kernel_stack_top(tsk) ; i++ )
	{
		if ( !i )
			printk("check_stack(): i = %p\n", i);
		if ( *i != 0 )
		{
			/* only notify if it's less than 900 bytes */
			if ( (i - (unsigned long *)task_top(tsk))  < 900 )
				printk("%d bytes free on stack\n",
				       i - (unsigned long *)task_top(tsk));
			break;
		}
	}
#endif

	if (ret)
	{
		panic("bad kernel stack");
	}
	return(ret);
}
#endif /* CHECK_STACK */

/*
 * Set up a thread for executing a new program
 */
void start_thread(struct pt_regs *regs, unsigned long nip, unsigned long sp)
{
/* XXX this is all wrong */
	printk ("start thread\n");
        set_fs(USER_DS);
        regs->psw.flags = USER_PSW;
        regs->psw.addr = nip;
        regs->irregs.r13 = sp;
}

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
void
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
        printk("%s/%d -> %s/%d PSW 0x%x 0x%x cpu %d root %x/%x\n",
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
	_lctl1 (new_tss->regs->cr1.raw);

	/* switch kernel stack pointers */
	_set_TCA ((unsigned long) &(new_tss->tca[0]));
	old_tss->ksp = _get_SP();
	_set_SP (new_tss->ksp);


	// note "s" is from a different stack ... 
        __restore_flags (s);
}


void exit_thread(void)
{
}

void flush_thread(void)
{
}

void
release_thread(struct task_struct *t)
{
}

void 
i370_sys_exit (void) 
{
	asm volatile ("SVC	23"); /* fault on purpose */
}

/*
 * Copy a thread..
 * mostly wrong .... but give it a shot
 * What this function does/needs to do:
 * -- Copy architecture-dependent parts of the task structure.
 * -- Set up the user stack pointer
 * -- Set up page tables ??? !!
 * -- Load CR1 with page table origin ??!!
 *
 * argument p is the copy_to_p copy_from is "current"
 * usp and regs are what was pass to do_fork below ...
 * return 0 if success, non-zero if not
 */
int
copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
            struct task_struct * p, struct pt_regs * regs)
{
	i370_elf_stack_t *srcsp, *dstsp, *this_frame;
	unsigned long delta;

	printk("copy thread \n");

	/* copy the kernel stack, and fix up the entries in it,
	 * so that it unwinds properly in the copied thread.
	 */
	srcsp = (i370_elf_stack_t *) (((char *) current) + sizeof (struct task_struct));
	dstsp = (i370_elf_stack_t *) (((char *) p) + sizeof (struct task_struct));
	this_frame = (i370_elf_stack_t *) _get_SP();
	delta = ((unsigned long) srcsp) - ((unsigned long) dstsp);
	p->tss.ksp = (unsigned long) dstsp;

	memcpy (dstsp, srcsp, this_frame->stack_top - p->tss.ksp);
	do {
		dstsp -> caller_r12 = &(p->tss.tca[0]);
		dstsp -> caller_sp = srcsp->caller_sp - delta;
		dstsp -> callee_sp = srcsp->callee_sp - delta;
		dstsp -> stack_top = srcsp->stack_top - delta;
		srcsp = (i370_elf_stack_t *) (srcsp -> stack_top);
		dstsp = (i370_elf_stack_t *) (dstsp -> stack_top);
	} while (srcsp != this_frame);

	/* if fork was from SVC, child gets a return value of zero.
	 * Do this by inserting a zero in reg fifteen of the SVC
	 * saved regs (in the SVC stack frame).
	 */
	regs -> irregs.r15 = 0;

	/* XXX what about page tables ?? */
	return 0;
}

/* 
 * note:
 * do_fork will call copy_thread, passing sp and regs to it
 */


asmlinkage int 
i370_sys_clone (unsigned long clone_flags,
                         struct pt_regs *regs)
{
        int res;
	printk ("sys clone \n");
	asm volatile ("SVC	67"); /* fault on purpose */
        lock_kernel();
        res = do_fork(clone_flags, regs->irregs.r13, regs);

#ifdef __SMP__
        /* When we clone the idle task we keep the same pid but
         * the return value of 0 for both causes problems.
         * -- Cort
         */
        if ((current->pid == 0) && (current == &init_task))
                res = 1;
#endif /* __SMP__ */
/*
 * XXX not clear on this .. looks to me like both the parent and the child
 * unlock ... is that OK ???
 * also ... do_fork() calls lock_kernel, i.e. it gets locked twice... 
 * is that OK ???
 */
        unlock_kernel();
        return res;
}

/*
 * i370_kernel_thread ... creates a new kernel thread.
 * Called during initialization, and during module load to create
 * misc important kernel threads. 
 *
 * The child is supposed to then call the arg fn(), and never return.
 * if for anyreason fn() returns, the child thread is supposed to be 
 * cleaned up and discarded.  Thus we don't have to worry about copying
 * all of the kernel stack; simply setting up a minimally configured
 * stack should do.
 * 
 */

long 
i370_kernel_thread(unsigned long flags, int (*fn)(void *), void *args)  
{
	long pid;
	printk ("create kernel_thread\n");
        pid = do_fork(flags, 0, current->tss.regs);
        if (pid) return pid;
	fn (args);
	while (1) { i370_sys_exit(); } 
	return 0;
}


int dump_fpu(struct pt_regs *regs, elf_fpregset_t *fpregs) { 
	printk ("dump fpu \n");
	return 0;
}


