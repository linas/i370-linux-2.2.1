
#include <linux/sched.h>
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
	printk ("dohh show more dohh here ...\n");
        printk("PSW-bits: %08lX PSW-ia: %08lX \n",
               regs->psw_bits, regs->psw_ia);

}

void
print_backtrace(unsigned long *sp)
{
//        int cnt = 0;
//        unsigned long i;

        printk("Call backtrace: ");
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

