#ifndef _I370_PTRACE_H
#define _I370_PTRACE_H

/*
 * This struct defines the way the registers are stored on the
 * kernel stack during a system call or other kernel entry.
 *
 * this should only contain volatile regs
 *
 * Note that the offsets of the fields in this struct correspond with
 * the PT_* values below.  
 */

#ifndef __ASSEMBLY__

/* Program Status Word */
struct _i370_psw_s {
	unsigned long flags;
	unsigned long addr;
} __attribute__ ((aligned (8)));
typedef struct _i370_psw_s psw_t __attribute__ ((aligned (8)));

/* ---------------------------------------------------------------- */
/* Control register zero bit definitions */
struct _i370_cr0_s {
	/* MSB */
	unsigned long :1;	/* unused */
	unsigned long ssmc:1;	/* SSM suppression control */
	unsigned long todc:1;	/* TOD clock sync control */
	unsigned long lapc:1;	/* low address protection control */
	unsigned long eac:1;	/* extraction authority control */
	unsigned long ssc:1;	/* secondary space control */
	unsigned long fpoc:1;	/* fetch protection override control */
	unsigned long spoc:1;	/* storage protection override control */
	unsigned long tf:5;	/* address translation format */
	unsigned long :1;	/* unused */
	unsigned long vc:1;	/* vector control */
	unsigned long asfc:1;	/* adress space function control */
	unsigned long masm:1;	/* malfunction alert subclass mask */
	unsigned long essm:1;	/* emergency signal subclass mask */
	unsigned long ecsm:1;	/* external call subclass mask */
	unsigned long todsm:1;	/* TOD-clock sync check subclass mask */
	unsigned long clksm:1;	/* clock comparator subclass mask */
	unsigned long cpusm:1;	/* cpu timer subclass mask */
	unsigned long sssm:1;	/* service siganl subclass mask */
	unsigned long :2;	/* unused */
	unsigned long iksm:1;	/* interrupt key subclass mask */
	unsigned long :6;	/* unused */
	/* LSB */
};
typedef struct _i370_cr0_s i370_cr0_t;

union _i370_cr0_u {
	i370_cr0_t    bits;
	unsigned long raw;
};
typedef union _i370_cr0_u cr0_t;

/* ---------------------------------------------------------------- */
/* Control register one bit definitions */
struct _i370_cr1_s {
	/* MSB */
	unsigned long pssec:1;	/* primary space-switch event control */
	unsigned long psto:19;	/* primary segment table origin */
	unsigned long :2;	/* unused */
	unsigned long psgc:1;	/* primary subspace group control */
	unsigned long ppsc:1;	/* primary private space control */
	unsigned long psaec:1;	/* primary storage alteration event control */
	unsigned long pstl:7;	/* primary segment table length */
	/* LSB */
};
typedef struct _i370_cr1_s i370_cr1_t;

union _i370_cr1_u {
	i370_cr1_t    bits;
	unsigned long raw;
};
typedef union _i370_cr1_u cr1_t;

/* ---------------------------------------------------------------- */
/* Control register six bit definitions */
struct _i370_cr6_s {
	/* MSB */
	unsigned long iosm:8;	/* I/O Interruption subclass mask */
	unsigned long :24;	/* unused */
	/* LSB */
};
typedef struct _i370_cr6_s i370_cr6_t;

union _i370_cr6_u {
	i370_cr6_t    bits;
	unsigned long raw;
};
typedef union _i370_cr6_u cr6_t;
#endif /* __ASSEMBLY__ */

/* ---------------------------------------------------------------- */
/* The current i370-elf stack entry.  This corresponds to 
 * what the current gcc/egcs generates, but is subject to 
 * change (!?).  
 *
 * Function args immediately follow the stack; after that
 * come local variables.  Normally, r13 points at the base 
 * of the stack, while r11 points just after the top.
 *
 * If the subroutine returns by value something larger
 * then 4 bytes, then the caller should reserve space
 * for the returned value; upon entry, r1 points at
 * the this area. Returned values 4 bytes or smaller
 * are returned in r15, else r15 is set to point at the
 * returned value.
 *
 * r11 is the (top of the) stack pointer
 * r13 is the frame pointer (aka bottom of the stack).
 * r10 is the static chain register
 * r12 is the .data address literal base pointer
 * r3  is the .text address literal (branch) base register
 * r1 is the struct value return pointer
 * r14 is the link register
 * r15 is the value return register
 */

/* The MIN_STACK_SIZE is the smallest possible stack size: 
 * just the size of struct i370_elf_stack, and no args, no local vars.
 * The MAX_STACK_SIZE is the largest possible stack size: 
 * the size of struct i370_elf_stack, and 100 args (of 4 bytes each)
 * This limit on the max number of arguments sounds reasonable, right !?!?
 */
#define MIN_STACK_SIZE 88
#define MAX_STACK_SIZE 488


#ifndef __ASSEMBLY__
struct _i370_elf_stack_s {
	unsigned long	page_table;		/* 0 */
	unsigned long	unused1;		/* 4 */
	unsigned long	caller_sp;		/* 8 */
	unsigned long	caller_r14;		/* 12 */
	unsigned long	caller_r15;		/* 16 */
	unsigned long	caller_r0;		/* 20 */
	unsigned long	caller_r1;		/* 24 */
	unsigned long	caller_r2;		/* 28 */
	unsigned long	caller_r3;		/* 32 */
	unsigned long	caller_r4;		/* 36 */
	unsigned long	caller_r5;		/* 40 */
	unsigned long	caller_r6;		/* 44 */
	unsigned long	caller_r7;		/* 48 */
	unsigned long	caller_r8;		/* 52 */
	unsigned long	caller_r9;		/* 56 */
	unsigned long	caller_r10;		/* 60 */
	unsigned long	callee_sp;		/* 64 */
	unsigned long	caller_r12;		/* 68 */
	unsigned long	unused2;	 	/* 72 */ 
	unsigned long	unused3;		/* 76 */
	unsigned long	scratch1;		/* 80 */
	unsigned long	scratch2;		/* 84 */
};
typedef struct _i370_elf_stack_s i370_elf_stack_t;

/* ---------------------------------------------------------------- */

struct _i370_irregs_s {
	unsigned long	r11;
	unsigned long	r12;
	unsigned long	r13;
	unsigned long	r14;
	unsigned long	r15;
	unsigned long	r0;
	unsigned long	r1;
	unsigned long	r2;
	unsigned long	r3;
	unsigned long	r4;
	unsigned long	r5;
	unsigned long	r6;
	unsigned long	r7;
	unsigned long	r8;
	unsigned long	r9;
	unsigned long	r10;
};
typedef struct _i370_irregs_s irregs_t;


/* ---------------------------------------------------------------- */
/* This structure (i370_interrupt_state_t) stores just enough
 * info to be able to take multiple nested interrupts, and move
 * on.  The rest of the interrupt state gets stored on the stack.
 * Per-thread state gets stored in the thread_struct and task_struct.
 *
 * This struct is created and manipulated with assembly code in 
 * head.S and therefore must not be changed without making corresponding
 * changes in head.S (such as SF_SIZE)
 */
typedef struct _i370_interrupt_state_s i370_interrupt_state_t;

struct _i370_interrupt_state_s {
	psw_t   	psw;	/* process status word */
	irregs_t	irregs;	/* some of the GPR's */
	cr0_t		cr0;	/* control register 0 */
	cr1_t		cr1;	/* control register 1 */
	i370_interrupt_state_t *oldregs;	/* backchain */

/* XXX all wrong for 370 but we need something like this? */
//	unsigned long orig_gpr3; /* Used for restarting system calls */
//	unsigned long trap;	/* XXXXX Reason for being here */
//	unsigned long result;   /* Result of a system call */
};

#define pt_regs _i370_interrupt_state_s

#define INIT_REGS { 			\
	{0,0},	/* psw */ 		\
	{0,0,0,0,0,0,0,0, /* irregs */	\
	 0,0,0,0,0,0,0,0}, /* irregs */	\
	0,0, /* cr */			\
	0}
#endif /* __ASSEMBLY__ */

/* These defines map the struct _i370_interrupt_state_s .
 * This is used by the assembly code to find things.
 */
#define IR_PSW          0x0
#define IR_R11          0x8
#define IR_R12          0xc
#define IR_R13          0x10
#define IR_R14          0x14
#define IR_R15          0x18
#define IR_R0           0x1c
#define IR_R1           0x20
#define IR_R2           0x24
#define IR_R3           0x28    
#define IR_R4           0x2c
 
#define IR_R5           0x30
#define IR_R6           0x34
#define IR_R7           0x38
#define IR_R8           0x3c
#define IR_R9           0x40
#define IR_R10          0x44
 
#define IR_CR0          0x48
#define IR_CR1          0x4c
#define IR_OLDREGS      0x50

#ifndef __ASSEMBLY__
/* ---------------------------------------------------------------- */

#define instruction_pointer(regs) (((regs)->psw.addr) & 0x7fffffff)
#define user_mode(regs) ((regs)->psw.flags & 0x10000)

#endif /* __ASSEMBLY__ */

#endif

