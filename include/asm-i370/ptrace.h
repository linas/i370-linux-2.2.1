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
};
typedef struct _i370_psw_s psw_t;


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

/*
 * NOTE that this structure is accessed in binary in head.S and
 * changes to it may break the exception handling code. Modify
 * at your own risk.
 */
struct _i370_regs_s {
	psw_t         psw;	/* process status word */
	unsigned long gpr[16];  /* general purpose regs */
	double   fpr[4];        /* floating point regs */
	cr0_t    cr0;		/* control register 0 */
	cr1_t    cr1;		/* control register 1 */

/* XXX all wrong for 370 but we need something like this? */
//	unsigned long orig_gpr3; /* Used for restarting system calls */
//	unsigned long trap;	/* XXXXX Reason for being here */
//	unsigned long result;   /* Result of a system call */
};

typedef struct _i370_regs_s i370_regs_t;

#define pt_regs _i370_regs_s


#endif

#define STACK_FRAME_OVERHEAD	148	/* size of minimum stack frame */

/* Size of stack frame allocated when calling signal handler. */
#define __SIGNAL_FRAMESIZE	64   /* XXX whoa all wrong */

#define instruction_pointer(regs) (((regs)->psw.addr) & 0x7fffffff)
#define user_mode(regs) ((regs)->psw.flags & 0x10000)

/*
 * Offsets used by 'ptrace' system call interface.
 * These can't be changed without breaking binary compatibility
 */
#define PT_R0	0
#define PT_R1	1
#define PT_R2	2
#define PT_R3	3
#define PT_R4	4
#define PT_R5	5
#define PT_R6	6
#define PT_R7	7
#define PT_R8	8
#define PT_R9	9
#define PT_R10	10
#define PT_R11	11
#define PT_R12	12
#define PT_R13	13
#define PT_R14	14
#define PT_R15	15
#define PT_R16	16

#define PT_FPR0	18	/* each FP reg occupies 2 slots in this space */

#define PT_PSW	25

#endif

