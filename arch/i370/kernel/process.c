
/*
 * mostly copied from the ppc implementation
 * XXX this is fairly broken for the 370
 */

#include <linux/sched.h>

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
   int i;

   for (i=0; i<16; i+=2) {
      printk ("GPR %d: %08lX   GPR %d %08lX \n", 
         i, regs->gpr[i], i+1, regs->gpr[i+1]);
   }
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
	for ( i = (unsigned long *)task_top(tsk) ; i < kernel_stack_top(tsk) ; i++ )
	{
		if ( !i )
			printk("check_stack(): i = %p\n", i);
		if ( *i != 0 )
		{
			/* only notify if it's less than 900 bytes */
			if ( (i - (unsigned long *)task_top(tsk))  < 900 )
				printk("%d bytes free on stack\n",
				       i - task_top(tsk));
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
	printk ("start thread\n");
        set_fs(USER_DS);
        regs->psw.flags = USER_PSW;
        regs->psw.addr = nip;
        regs->gpr[13] = sp;
}

// switch_to does the task switching.  
// I'm not to clear on what it needs to do ... 
// ... needs to switch the stack pointer, at least ... 
// ... change addressing modes (potentially, from real 
//     to primary or v.v.  ...
// ... modify the segment table CR1 and mabye CR3 key  ...
// ... Purge TLB ...
void
switch_to(struct task_struct *prev, struct task_struct *new)
{
	struct thread_struct *new_tss, *old_tss;
	i370_regs_t *oldregs, *newregs;

//        int s = _disable_interrupts();

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

	current = new;

	/* switch control registers */
	/* cr1 contains the segment table origin */
	_lctl1 (new_tss->regs->cr1.raw);

	/* switch kernel stack pointers */
	old_tss->ksp = _get_SP();
	_set_SP (new_tss->ksp);

#if 0
	oldregs = old_tss->regs;
	newregs = new_tss->regs;

	/* I'm going to try to do this inline, and see if I can
         * pull this off. It might be too hard ...
	* BTW, this code is totally and completely wrong .... 
	 */
	asm volatile (
	"bcr	15,0;"		/* sync */
	"stm	r0,r15,%0;"	/* save all registers */

	"lm	r0,r15,%1;"	/* restore all registers */
	"bcr	15,0;"		/* sync */
	: "=m" (oldregs)
	: "m" (newregs)
	: "memory",		/* disable optimization around this */ 
	  "r0"			/* we blow r0 for psw */

	);
#endif


//        _switch(old_tss, new_tss, new->mm->context);
//        _enable_interrupts(s);
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

/*
 * Copy a thread..
 * mostly wrong .... but give it a shot
 * What this function does/needs to do:
 * -- Copy architecture-dependent parts of the task structure.
 * -- Set up the user stack pointer
 * -- Set up page tables ??? !!
 * -- Load CR1 with page table origin ??!!
 */
int
copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
            struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs *childregs;

	printk("copy thread \n");

	childregs = p->tss.regs;
	*childregs = *regs;

	childregs->gpr[15] = 0;		/* result from 'fork' ??? */
	childregs->gpr[13] = usp;	/* user-space stack pointer */

	/* XXX what about page tables ?? */
	return 0;
}

/*
 * i370_kernel_thread ...
 * I think this one needs to copy  ...?
 */

long 
i370_kernel_thread(unsigned long flags, int (*fn)(void *), void *args)  
{
	printk ("kernel_thread\n");
	asm volatile ("SVC 0xbb"); // fault
	return 0;
}


int dump_fpu(struct pt_regs *regs, elf_fpregset_t *fpregs) { 
	printk ("dump fpu \n");
	return 0;
}


