#ifndef __ASM_I370_PROCESSOR_H
#define __ASM_I370_PROCESSOR_H

#include <linux/config.h>

#include <asm/ptrace.h>

/* XXX this is complete garbage */

/* Bit encodings for Machine State Register (MSR) */
#define MSR_POW		(1<<18)		/* Enable Power Management */
#define MSR_TGPR	(1<<17)		/* TLB Update registers in use */
#define MSR_ILE		(1<<16)		/* Interrupt Little-Endian enable */
#define MSR_EE		(1<<15)		/* External Interrupt enable */
#define MSR_PR		(1<<14)		/* Supervisor/User privilege */
#define MSR_FP		(1<<13)		/* Floating Point enable */
#define MSR_ME		(1<<12)		/* Machine Check enable */
#define MSR_FE0		(1<<11)		/* Floating Exception mode 0 */
#define MSR_SE		(1<<10)		/* Single Step */
#define MSR_BE		(1<<9)		/* Branch Trace */
#define MSR_FE1		(1<<8)		/* Floating Exception mode 1 */
#define MSR_IP		(1<<6)		/* Exception prefix 0x000/0xFFF */
#define MSR_IR		(1<<5)		/* Instruction MMU enable */
#define MSR_DR		(1<<4)		/* Data MMU enable */
#define MSR_RI		(1<<1)		/* Recoverable Exception */
#define MSR_LE		(1<<0)		/* Little-Endian enable */

#define MSR_		MSR_ME|MSR_RI
#define MSR_KERNEL      MSR_|MSR_IR|MSR_DR
#define MSR_USER	MSR_KERNEL|MSR_PR|MSR_EE


/* fpscr settings */
#define FPSCR_FX        (1<<31)
#define FPSCR_FEX       (1<<30)

#define _MACH_390      1

#define _GLOBAL(n)\
	.globl n;\
n:

#define	TBRU	269	/* Time base Upper/Lower (Reading) */
#define	TBRL	268
#define TBWU	284	/* Time base Upper/Lower (Writing) */
#define TBWL	285
#define	XER	1
#define LR	8
#define CTR	9
#define HID0	1008	/* Hardware Implementation */
#define PVR	287	/* Processor Version */
#define IBAT0U	528	/* Instruction BAT #0 Upper/Lower */
#define IBAT0L	529
#define IBAT1U	530	/* Instruction BAT #1 Upper/Lower */
#define IBAT1L	531
#define IBAT2U	532	/* Instruction BAT #2 Upper/Lower */
#define IBAT2L	533
#define IBAT3U	534	/* Instruction BAT #3 Upper/Lower */
#define IBAT3L	535
#define DBAT0U	536	/* Data BAT #0 Upper/Lower */
#define DBAT0L	537
#define DBAT1U	538	/* Data BAT #1 Upper/Lower */
#define DBAT1L	539
#define DBAT2U	540	/* Data BAT #2 Upper/Lower */
#define DBAT2L	541
#define DBAT3U	542	/* Data BAT #3 Upper/Lower */
#define DBAT3L	543
#define DMISS	976	/* TLB Lookup/Refresh registers */
#define DCMP	977
#define HASH1	978
#define HASH2	979
#define IMISS	980
#define ICMP	981
#define RPA	982
#define SDR1	25	/* MMU hash base register */
#define DAR	19	/* Data Address Register */
#define SPR0	272	/* Supervisor Private Registers */
#define SPRG0   272
#define SPR1	273
#define SPRG1   273
#define SPR2	274
#define SPRG2   274
#define SPR3	275
#define SPRG3   275
#define DSISR	18
#define SRR0	26	/* Saved Registers (exception) */
#define SRR1	27
#define IABR	1010	/* Instruction Address Breakpoint */
#define DEC	22	/* Decrementer */
#define EAR	282	/* External Address Register */
#define L2CR	1017    /* I370 750 L2 control register */

#define THRM1	1020
#define THRM2	1021
#define THRM3	1022
#define THRM1_TIN 0x1
#define THRM1_TIV 0x2
#define THRM1_THRES (0x7f<<2)
#define THRM1_TID (1<<29)
#define THRM1_TIE (1<<30)
#define THRM1_V   (1<<31)
#define THRM3_E   (1<<31)

/* Segment Registers */
#define SR0	0
#define SR1	1
#define SR2	2
#define SR3	3
#define SR4	4
#define SR5	5
#define SR6	6
#define SR7	7
#define SR8	8
#define SR9	9
#define SR10	10
#define SR11	11
#define SR12	12
#define SR13	13
#define SR14	14
#define SR15	15

#ifndef __ASSEMBLY__
/*
 * If we've configured for a specific machine set things
 * up so the compiler can optimize away the other parts.
 * -- Cort
 */

extern int _machine;

struct task_struct;
void start_thread(struct pt_regs *regs, unsigned long nip, unsigned long sp);
void release_thread(struct task_struct *);

/*
 * Bus types
 */
#define MCA_bus 0
#define MCA_bus__is_a_macro /* for versions in ksyms.c */

/* Lazy FPU handling on uni-processor */
extern struct task_struct *last_task_used_math;

/*
 * this is the minimum allowable io space due to the location
 * of the io areas on prep (first one at 0x80000000) but
 * as soon as I get around to remapping the io areas with the BATs
 * to match the mac we can raise this. -- Cort
 */
#define TASK_SIZE	(0x80000000UL)

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	(TASK_SIZE / 8 * 3)

typedef struct {
	unsigned long seg;
} mm_segment_t;

struct thread_struct {
	unsigned long	ksp;		/* Kernel stack pointer */
	unsigned long	*pg_tables;	/* Base of page-table tree */
	unsigned long	wchan;		/* Event task is sleeping on */
	struct pt_regs	*regs;		/* Pointer to saved register state */
	mm_segment_t	fs;		/* for get_fs() validation */
	signed long     last_syscall;
	double		fpr[4];		/* Complete floating point set */
	unsigned long	fpscr_pad;	/* fpr ... fpscr must be contiguous */
	unsigned long	fpscr;		/* Floating point status */
	unsigned long	smp_fork_ret;
};

#define INIT_SP		(sizeof(init_stack) + (unsigned long) &init_stack)

#define INIT_TSS  { \
	INIT_SP, /* ksp */ \
	(unsigned long *) swapper_pg_dir, /* pg_tables */ \
	0, /* wchan */ \
	(struct pt_regs *)INIT_SP - 1, /* regs */ \
	KERNEL_DS, /*fs*/ \
	0, /* last_syscall */ \
	{0}, 0, 0, 0 \
}

/*
 * Note: the vm_start and vm_end fields here should *not*
 * be in kernel space.  (Could vm_end == vm_start perhaps?)
 */
#define INIT_MMAP { &init_mm, 0, 0x1000, NULL, \
		    PAGE_SHARED, VM_READ | VM_WRITE | VM_EXEC, \
		    1, NULL, NULL }

/*
 * Return saved PC of a blocked thread. For now, this is the "user" PC
 */
static inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	return (t->regs) ? t->regs->nip : 0;
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

#endif /* ndef ASSEMBLY*/

  
#endif /* __ASM_I370_PROCESSOR_H */







