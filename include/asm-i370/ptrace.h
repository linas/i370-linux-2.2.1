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
	/* LSB */
	unsigned long :6;	/* unused */
	unsigned long iksm:1;	/* interrupt key subclass mask */
	unsigned long :2;	/* unused */
	unsigned long sssm:1;	/* service siganl subclass mask */
	unsigned long cpusm:1;	/* cpu timer subclass mask */
	unsigned long clksm:1;	/* clock comparator subclass mask */
	unsigned long todsm:1;	/* TOD-clock sync check subclass mask */
	unsigned long ecsm:1;	/* external call subclass mask */
	unsigned long essm:1;	/* emergency signal subclass mask */
	unsigned long masm:1;	/* malfunction alert subclass mask */
	unsigned long asfc:1;	/* adress space function control */
	unsigned long vc:1;	/* vector control */
	unsigned long :1;	/* unused */
	unsigned long tf:5;	/* address translation format */
	unsigned long spoc:1;	/* storage protection override control */
	unsigned long fpoc:1;	/* fetch protection override control */
	unsigned long ssc:1;	/* secondary space control */
	unsigned long eac:1;	/* extraction authority control */
	unsigned long lapc:1;	/* low address protection control */
	unsigned long todc:1;	/* TOD clock sync control */
	unsigned long ssmc:1;	/* SSM suppression control */
	unsigned long :1;	/* unused */
	/* MSB */
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
	/* LSB */
	unsigned long pstl:7;	/* primary segment table length */
	unsigned long psaec:1;	/* primary storage alteration event control */
	unsigned long ppsc:1;	/* primary private space control */
	unsigned long psgc:1;	/* primary subspace group control */
	unsigned long :2;	/* unused */
	unsigned long psto:19;	/* primary segment table origin */
	unsigned long pssec:1;	/* primary space-switch event control */
	/* MSB */
};
typedef struct _i370_cr1_s i370_cr1_t;

union _i370_cr1_u {
	i370_cr1_t    bits;
	unsigned long raw;
};
typedef union _i370_cr1_u cr1_t;

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
};
typedef struct _i370_irregs_s irregs_t;


/* ---------------------------------------------------------------- */
/* This structure (i370_interrupt_state_t) stores just enough
 * info to be able to take multiple nested interrupts, and move
 * on.  The rest of the interrupt state gets stored on the stack.
 * Per-thread state gets stored in the thread_struct and task_struct.
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
	{0,0,0,0,0,0,0,0}, /* irregs */	\
	0,0, /* cr */			\
	0}

/* ---------------------------------------------------------------- */

#define instruction_pointer(regs) (((regs)->psw.addr) & 0x7fffffff)
#define user_mode(regs) ((regs)->psw.flags & 0x10000)

#endif

#endif

