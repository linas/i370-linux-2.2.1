#ifndef _I370_PTRACE_H
#define _I370_PTRACE_H

/*
 * This struct defines the way the registers are stored on the
 * kernel stack during a system call or other kernel entry.
 *
 * this should only contain volatile regs
 *
 * Since this is going on the stack, *CARE MUST BE TAKEN* to insure
 * that the overall structure is a multiple of 16 bytes in length.
 * ??? huh ??? explain this  !
 *
 * Note that the offsets of the fields in this struct correspond with
 * the PT_* values below.  
 */

#ifndef __ASSEMBLY__

struct psw_s {
	unsigned long flags;
	unsigned long addr;
};

typedef struct psw_s psw_t;

/*
 * NOTE that this structure is accessed in binary in head.S and
 * changes to it may break the exception handling code. Modify
 * at your own risk.
 */
struct pt_regs {
	psw_t         psw;	/* process status word */
	unsigned long gpr[16];
	unsigned long fpr[8];    /* do we really need to save these ? */
	unsigned long pad[2];
/* XXX all wrong for 370 but we need something like this? */
//	unsigned long orig_gpr3; /* Used for restarting system calls */
//	unsigned long trap;	/* XXXXX Reason for being here */
//	unsigned long result;   /* Result of a system call */
};
#endif

#define STACK_FRAME_OVERHEAD	16	/* size of minimum stack frame */

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
#ifdef __KERNEL__
#define PT_ORIG_R3 34
#endif

#endif

