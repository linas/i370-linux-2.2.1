
/*
 * XXX this is mostly all wrong for the 370
 */

#include <linux/sched.h>
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

/*
 * Set up a thread for executing a new program
 */
void start_thread(struct pt_regs *regs, unsigned long nip, unsigned long sp)
{
	printk ("start thread\n");
}

// switch_to does the task switching.  
// It needs to save registers and the psw,
// unload the tasks segment table CR1 and mabye CR3 key ?? 
// and then do the opposite fore the new process ...
void
switch_to(struct task_struct *prev, struct task_struct *new)
{
	printk("swith_to\n");
}


void exit_thread(void)
{
//        if (last_task_used_math == current)
//                 last_task_used_math = NULL;
}

void flush_thread(void)
{
 //       if (last_task_used_math == current)
//                last_task_used_math = NULL;
}

void
release_thread(struct task_struct *t)
{
}

/*
 * Copy a thread..
 */
int
copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
            struct task_struct * p, struct pt_regs * regs)
{
   /* XXX */
   return 0;
}

int dump_fpu(struct pt_regs *regs, elf_fpregset_t *fpregs) { return 0;}
