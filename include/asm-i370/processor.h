#ifndef __ASM_I370_PROCESSOR_H
#define __ASM_I370_PROCESSOR_H

#include <linux/config.h>

#include <asm/ptrace.h>

/* Fixed Storage Locations
 * Locations of various process status words (PSW's)
 * in low real-mode memory.
 */
#define IPL_PSW_NEW	0x0	/* Restart new PSW */
#define IPL_PSW_OLD	0x8	/* Restart old PSW */
#define IPL_CCW		0x10
#define	EXTERN_PSW_OLD	0x18	/* External exception old PSW */
#define SVC_PSW_OLD	0x20	/* Service Call old PSW */
#define PROG_PSW_OLD	0x28	/* Program exception old PSW */
#define MACH_PSW_OLD	0x30	/* Machine Check old PSW */
#define IO_PSW_OLD	0x38	/* IO Exception old PSW */
#define EXTERN_PSW_NEW	0x58	/* External interruption new PSW */
#define SVC_PSW_NEW	0x60	/* Service Call exception new PSW */
#define PROG_PSW_NEW	0x68	/* Program new PSW */
#define MACH_PSW_NEW	0x70	/* Machine Check new PSW */
#define IO_PSW_NEW	0x78	/* IO Exception new PSW */


#define _GLOBAL(n)\
	.globl n;\
n:

/* Bit encodings in the PSW */
#define PSW_PER		(1<<30)		/* Program Event Recording Mask */
#define PSW_DAT		(1<<26)		/* Address Translation Mode */
#define PSW_IO		(1<<25)		/* Input/Output Mask */
#define	PSW_EXTERN	(1<<24)		/* External Mask */
#define PSW_KEY(key)	((key&0xf)<<20)	/* PSW Protection Key */
#define PSW_MACH	(1<<18)		/* Machine Check Mask */
#define PSW_WAIT	(1<<17)		/* Wait State */
#define PSW_PROB	(1<<16)		/* Problem State (user Mode) */
#define PSW_PRIMARY	PSW_DAT || (0x0000)	/* Primary Space Mode */
#define PSW_AR		PSW_DAT || (1<<14)	/* Access Register Mode */
#define PSW_SECONDARY	PSW_DAT || (2<<14)	/* Secondary Space Mode */
#define PSW_HOME	PSW_DAT || (3<<14)	/* Home Space Mode */


/* USER_PSW sets up flags for the user mode */
#define EN_PSW		PSW_IO | PSW_EXTERN | PSW_MACH | PSW_WAIT
#define USER_PSW	EN_PSW | PSW_DAT | PSW_PROB
#define KERN_PSW	EN_PSW | PSW_KEY(3)

/* XXX The rest of this file is garbage XXX */
/* copied from powerpc it should be trashed ... */


#ifndef __ASSEMBLY__
struct task_struct;
void start_thread(struct pt_regs *regs, unsigned long psw, unsigned long sp);
void release_thread(struct task_struct *);

/*
 *  ??? 
 */
#define TASK_SIZE	(0x80000000UL)

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	(TASK_SIZE)

typedef struct {
	unsigned long seg;
} mm_segment_t;

/* the thread_struct is inlined into the arch-independent task_struct */
/* ??? Its not at all clear to me when things should go here, and when thry
 * should go into pt_regs ... 
 * for example, the control regs ... here or in pt_regs ??
 */
struct thread_struct {
	unsigned long	ksp;		/* Kernel stack pointer */
	unsigned long	*pg_tables;	/* Base of page-table tree */
	unsigned long	wchan;		/* Event task is sleeping on */
	struct pt_regs	*regs;		/* Pointer to saved register state */
	mm_segment_t	fs;		/* for get_fs() validation */
	signed long     last_syscall;
	double		fpr[4];		/* Complete floating point set */
	unsigned long	smp_fork_ret;
};

#define INIT_SP		(sizeof(init_stack) + (unsigned long) &init_stack)

#define INIT_TSS  { \
	INIT_SP, /* ksp */ \
	(unsigned long *) swapper_pg_dir, /* pg_tables */ \
	0, /* wchan */ \
	(struct pt_regs *)INIT_SP, /* regs */ \
	KERNEL_DS, /*fs*/ \
	0, /* last_syscall */ \
	{0.0,0.0,0.0,0.0},  0 \
}

/*
 * Note: the vm_start and vm_end fields here should *not*
 * be in kernel space.  (Could vm_end == vm_start perhaps?)
 */
#define INIT_MMAP { &init_mm, 0, 0x1000, NULL, \
		    PAGE_SHARED, VM_READ | VM_WRITE | VM_EXEC, \
		    1, NULL, NULL }

/*
 * Return saved instruction address (PSW) of a blocked thread. 
 */
static inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	return (t->regs) ? (t->regs->psw.addr &0x7fffffff) : 0;
}

#define copy_segments(nr, tsk, mm)	do { } while (0)
#define release_segments(mm)		do { } while (0)
#define forget_segments()		do { } while (0)

/*
 * NOTE! The task struct and the stack go together
 */
#define alloc_task_struct() \
	((struct task_struct *) __get_free_pages(GFP_KERNEL,1))
#define free_task_struct(p)	free_pages((unsigned long)(p),1)

/* in process.c - for early bootup debug -- Cort */
int ll_printk(const char *, ...);
void ll_puts(const char *);

#define init_task	(init_task_union.task)
#define init_stack	(init_task_union.stack)

#endif /* endif ASSEMBLY*/

  
#endif /* __ASM_I370_PROCESSOR_H */







