#ifndef __ASM_I370_PROCESSOR_H
#define __ASM_I370_PROCESSOR_H

#include <linux/config.h>

#include <asm/asm.h>
#include <asm/ptrace.h>

/*------------------------------------------------------------*/
/* Prefix Page Fixed Storage Locations                        */
/* Locations of various process status words (PSW's)          */
/* in low real-mode memory.                                   */
/*------------------------------------------------------------*/
#define IPL_PSW_NEW	0x0	/* Restart new PSW */
#define IPL_PSW_OLD	0x8	/* Restart old PSW */
#define IPL_CCW1	0x8
#define IPL_CCW2	0x10
#define EXTERN_PSW_OLD	0x18	/* External exception old PSW */
#define SVC_PSW_OLD	0x20	/* Service Call old PSW */
#define PROG_PSW_OLD	0x28	/* Program exception old PSW */
#define MACH_PSW_OLD	0x30	/* Machine Check old PSW */
#define IO_PSW_OLD	0x38	/* IO Exception old PSW */
#define EXTERN_PSW_NEW	0x58	/* External interruption new PSW */
#define SVC_PSW_NEW	0x60	/* Service Call exception new PSW */
#define PROG_PSW_NEW	0x68	/* Program new PSW */
#define MACH_PSW_NEW	0x70	/* Machine Check new PSW */
#define IO_PSW_NEW	0x78	/* IO Exception new PSW */

#define PFX_EXT_SIG	0x80	/* External Interruption Service Signal */
#define PFX_EXT_CODE	0x84	/* External Interruption Code location */
#define PFX_SVC_CODE	0x88	/* Supervisor Call Code location */
#define PFX_PRG_CODE	0x8c	/* Program Interrrupt Code location */
#define PFX_PRG_TRANS	0x90	/* Program Interrrupt Translation Exception Code */
#define PFX_MCH_CODE	0xe8	/* Machine Check Interrrupt Code location */
#define PFX_MCH_CODE_LO	0xe8	/* Machine Check Interrrupt Code low word */
#define PFX_MCH_CODE_HI	0xec	/* Machine Check Interrrupt Code high word */

#define PFX_SUBSYS_ID	0xb8	/* Subsystem id word (subchannel) */
#define PFX_IO_PARM	0xbc	/* IO interruption paramter */

/*-----------------------------------------------------------------*/
/* Interrupt Scratch Areas...                                      */
/*-----------------------------------------------------------------*/
#define PRG_SAVE_CTX_REGS       0xe00   /* PC Context Save Area */
#define PRG_SAVE_CTX_PSW        0xe40
#define PRG_SAVE_CTX_CODE       0xe48
#define PRG_SAVE_CTX_TRANS      0xe4c
#define PRG_SAVE_CTX_C12        0xe50

/* TASK_STRUCT_SIZE should really be sizeof(struct task_struct) except
 * that we need to use this in assembly code.  So we're going to pad it
 * a bit, and go from there. Last I measured, sizeof(struct task_struct)
 * was 768 (!)
 */
#define TASK_STRUCT_SIZE	1024

#define OFFSET_KREGS    0xef8   /* Offset to kernel register pointer */
#define OFFSET_KSP      0xefc   /* Offset to kernel stack pointer */
#define INTERRUPT_BASE	0xf00	  /* Interrupt scratch area */

#define PC_INTERRUPT_BASE 0xf80 /* PC Interrupt scratch area */

/*------------------------------------------------------------*/
/* External Interruption Codes */
/*------------------------------------------------------------*/
#define EI_INTERRUPT_KEY	0x0040	/* Interrupt Key             */
#define EI_TOD_CLOCK_SYNC	0x1003	/* TOD clock sync check      */
#define EI_CLOCK_COMP		0x1004	/* Clock Comparator          */
#define	EI_CPU_TIMER		0x1005	/* CPU Timer                 */
#define	EI_MALFUNCTION		0x1200	/* Malfunction Alert         */
#define	EI_EMERGENCY		0x1201	/* Emerency Signal           */
#define	EI_CALL			0x1202	/* External Call             */
#define	EI_SERVICE		0x2401	/* Service Signal            */
#define	EI_IUCV			0x4000	/* IUCV Interrupt            */

/*------------------------------------------------------------*/
/* Bit encodings in the PSW */
/*------------------------------------------------------------*/
#define PSW_31BIT	0x80000000	/* 31-bit Addressing Mode    */
#define PSW_PER		0x40000000	/* Program Event Record Mask */
#define PSW_DAT		0x04000000	/* Address Translation Mode  */
#define PSW_IO		0x02000000	/* Input/Output Mask         */
#define	PSW_EXTERN	0x01000000	/* External Mask             */
#define PSW_KEY_MASK	0x00f00000	/* PSW Protection Key Mask   */
#define PSW_KEY(key)	((key&0xf)<<(31-11))   /* PSW Protection Key */
#define PSW_VALID	0x00080000	/* Must be set to one always */
#define PSW_MACH	0x00040000	/* Machine Check Mask        */
#define PSW_WAIT	0x00020000	/* Wait State                */
#define PSW_PROB	0x00010000	/* Problem State (User Mode) */
#define PSW_SPACE_MASK  0x0000c000	/* Space Mode Mask Bits      */
#define PSW_PRIMARY	0x00000000	/* Primary Space Mode        */
#define PSW_AR		0x00004000	/* Access Register Mode      */
#define PSW_SECONDARY	0x00008000	/* Secondary Space Mode      */
#define PSW_HOME	0x0000c000	/* Home Space Mode           */


/*-------------------------------------------------------------------*/
/* USER_PSW sets up flags for the user mode                          */
/* KERN_PSW sets up flags for the kernel w/ interrupts enabled       */
/* DISAB_PSW sets up flags for the kernel w/ interrupts disabled     */
/* HALT_PSW loads a disabled wait state (cpu halt)                   */
/*-------------------------------------------------------------------*/
#define EN_PSW		PSW_VALID | PSW_IO | PSW_EXTERN | PSW_MACH
/* XXX disable key 9 until the pte's are fixed up XXX */
/* #define USER_PSW	EN_PSW | PSW_DAT | PSW_PROB | PSW_KEY(9) */
#define USER_PSW	EN_PSW | PSW_DAT | PSW_PROB | PSW_KEY(6)
#define KERN_PSW	EN_PSW | PSW_KEY(6)
#define DISAB_PSW	PSW_VALID | PSW_KEY(6)
#define HALT_PSW	PSW_VALID | PSW_WAIT


/*------------------------------------------------------------*/
/* Program Interruption Codes                                 */
/*------------------------------------------------------------*/
#define PIC_OPERATION		0x01
#define PIC_PRIVLEDGED		0x02
#define PIC_EXECUTE		0x03
#define PIC_PROTECTION		0x04
#define PIC_ADDRESSING		0x05
#define PIC_SPECIFICATION	0x06
#define PIC_DATA		0x07
#define PIC_FIXED_OVERFLOW	0x08
#define PIC_FIXED_DIVIDE	0x09
#define PIC_DECIMAL_OVERFLOW	0x0a
#define PIC_DECIMAL_DIVIDE	0x0b
#define PIC_EXP_OVERFLOW	0x0c
#define PIC_EXP_UNDERFLOW	0x0d
#define PIC_SIGNIFICANCE	0x0e
#define PIC_FP_DIVIDE		0x0f
#define PIC_SEGEMENT_TRANS	0x10
#define PIC_PAGE_TRANS		0x11
#define PIC_TRANSLATION		0x12
#define PIC_SPECIAL_OP		0x13
#define PIC_PAGEX		0x14
#define PIC_OPERAND		0x15
#define PIC_TRACE_TABLE		0x16
#define PIC_ASN_TRANS		0x17
#define PIC_VECTOR_OP		0x19
#define PIC_SPACE_SWITCH	0x1c
#define PIC_SQROOT		0x1d
#define PIC_UNNORMALIZED	0x1e
#define PIC_PC_TRANS		0x1f
#define PIC_AFX_TRANS		0x20
#define PIC_ASX_TRANS		0x21
#define PIC_LX_TRANS		0x22
#define PIC_EX_TRANS		0x23
#define PIC_PRIMARY_AUTH	0x24
#define PIC_SECONDARY_AUTH	0x25
#define PIC_ALET_SPEC		0x28
#define PIC_ALEN_TRANS		0x29
#define PIC_ALE_SEQ		0x2a
#define PIC_ASTE_VALIDITY	0x2b
#define PIC_ASTE_SEQ		0x2c
#define PIC_EXTENDED_AUTH	0x2d
#define PIC_STACK_FULL		0x30
#define PIC_STACK_EMPTY		0x31
#define PIC_STACK_SPEC		0x32
#define PIC_STACK_TYPE		0x33
#define PIC_STACKOP		0x34
#define PIC_MONITOR		0x40
#define PIC_PER			0x80

#define MASK_TRXADDR  0x7ffff000
#define MASK_TRXVALID 0x00000004

#ifndef __ASSEMBLY__

/*------------------------------------------------------------*/
/* External interrupt structures                              */
/*------------------------------------------------------------*/

typedef struct
{
	short int ei_code;
	void (*ei_flih)(i370_interrupt_state_t *, unsigned short);
} ei_handler;

typedef struct
{
	short int iucv_path;        /* IUCV path identifier          */
	unsigned long iucv_uword;   /* User word                     */
	void  *iucv_buffer;         /* Pointer to the IUCV IPARML    */
} iucv_path;

/*------------------------------------------------------------*/
/* Program check interrupt structures                         */
/*------------------------------------------------------------*/

typedef struct
{
	short int pc_code;
	int (*pc_flih)(i370_interrupt_state_t *, unsigned long, unsigned short);
} pc_handler;

/*------------------------------------------------------------*/
/* CPU Details structure                                      */
/*------------------------------------------------------------*/

typedef struct
{
	unsigned long  CPU_address;  /* Address of CPU             */
	char  CPU_name[8];           /* An arbitrary name    */
	unsigned long CPU_status;    /* CPU status (from SIGP Sense)*/
	char  xxx[4];                /* Padding                 */
} CPU_t;

#define CPU_eqptchk 0x80000000    /* Equipment check     */
#define CPU_incstat 0x00000200    /* Incorrect state     */
#define CPU_invlprm 0x00000100    /* Invalid parameter */
#define CPU_extcall 0x00000080    /* External-call pending    */
#define CPU_stopped 0x00000040    /* Stopped             */
#define CPU_oprintv 0x00000020    /* Operator intervening     */
#define CPU_chkstop 0x00000010    /* Check stop               */
#define CPU_inoprtv 0x00000004    /* Inoperative             */
#define CPU_invlord 0x00000002    /* Invalid order         */
#define CPU_rcvrchk 0x00000001    /* Receiver check       */

#define SIGPSENS 0x01   /* Sense                   */
#define SIGPEXTC 0x02   /* External call           */
#define SIGPESIG 0x03   /* Emergency signal     */
#define SIGPSTRT 0x04   /* Start                   */
#define SIGPSTOP 0x05   /* Stop                     */
#define SIGPRSTA 0x06   /* Restart               */
#define SIGPSASS 0x09   /* Stop and store status    */
#define SIGPIRST 0x0B   /* Initial CPU reset   */
#define SIGPCRST 0x0C   /* CPU reset           */
#define SIGPSPFX 0x0D   /* Set prefix         */
#define SIGPSSAA 0x0E   /* Store status at address  */


/*------------------------------------------------------------*/
/* TASK_SIZE is the size of the effective address space for   */
/* one task which is 2GB for the 31-bit 390/ESA arch.         */
/* Note that userland binaries are loaded such that the       */
/* stack appears at the top of this range, and the executable */
/* at the bottom.                                             */
/*------------------------------------------------------------*/
#define TASK_SIZE	(0x80000000UL)

/*------------------------------------------------------------*/
/* XXX The rest of this file is sort of garbage user beware   */
/*------------------------------------------------------------*/

void i370_start_thread(struct pt_regs *regs, unsigned long pswa, unsigned long sp);
struct task_struct;
void release_thread(struct task_struct *);
#define start_thread(R,P,S)	i370_start_thread(R,P,S)

/*------------------------------------------------------------*/
/* This decides where the kernel will search for a free chunk */
/* of vm space during mmap's.                                 */
/*------------------------------------------------------------*/
#define TASK_UNMAPPED_BASE	(TASK_SIZE / 3)

/*--------------------------------------------------------------------*/
/* The thread_struct is inlined into the arch-independent task_struct */
/* Here's the deal:                                                   */
/* This sturcture contains (obviously) per-thread data that we need to*/
/* keep around.  This includes a pointer to per-interrupt data that   */
/* we need.  The per-interrupt data is in "struct pt_regs" aka        */
/* i370_interrupt_state.  This is by necessity a pointer into the     */
/* interrupt stack; with each new interrupt, this pointer is bumped   */
/* to the next stackframe etc. and upon each return from an interrupt */
/* it is unwound to the last.  If it is null, it can only mean that   */
/* this process is currently executing (as it would otherwise have    */
/* an interrupt context).                                             */
/*--------------------------------------------------------------------*/
typedef struct {
	unsigned long seg;
} mm_segment_t;

struct thread_struct {
	unsigned long	ksp;		/* Kernel stack pointer             */
	struct pt_regs *regs;		/* Pointer to saved interrupt state */
	unsigned long  *pg_tables;	/* Base of page-table tree          */
	cr0_t		cr0;		/* control register 0               */
	cr1_t		cr1;		/* control register 1               */
	double		fpr[16];	/* Complete floating point set      */
	int		in_slih;	/* set if we are in bottom half     */

	/* XXX what the stuff below???? why do we needed it ??? see PowerPC */
	unsigned long	wchan;		/* Event task is sleeping on        */
	mm_segment_t	fs;		/* for get_fs() validation          */
	signed long     last_syscall;
	unsigned long	smp_fork_ret;
};

#define init_task	(init_task_union.task)

/* the initial stack pointer is just above the top of the task struct */
#define init_stack	(((unsigned long)(&init_task_union.stack)) + TASK_STRUCT_SIZE)

#define INIT_SP		init_stack

#define INIT_TSS  { 						\
	INIT_SP, /* ksp */ 					\
	0, /* regs */ 						\
	(unsigned long *) swapper_pg_dir, /* pg_tables */ 	\
	0, /* cr0 */ 						\
	0, /* cr1 */ 						\
	{0.0, 0.0, 0.0, 0.0,   /* FPR's */ 			\
	 0.0, 0.0, 0.0, 0.0,   /* FPR's */ 			\
	 0.0, 0.0, 0.0, 0.0,   /* FPR's */ 			\
	 0.0, 0.0, 0.0, 0.0,}, /* FPR's */ 			\
	0, /* in_slih */ 					\
								\
	0, /* wchan */ 						\
	KERNEL_DS, /*fs*/ 					\
	0, /* last_syscall */ 					\
	0 							\
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
 * alloc_task_struct is only invoked inside of do_fork() in
 * the arch-indy code.
 */
#define alloc_task_struct() \
	((struct task_struct *) __get_free_pages(GFP_KERNEL,1))
#define free_task_struct(p)	free_pages((unsigned long)(p),1)

#endif /* endif ASSEMBLY*/

#endif /* __ASM_I370_PROCESSOR_H */
