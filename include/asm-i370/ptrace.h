#ifndef _I370_PTRACE_H
#define _I370_PTRACE_H

/*
 * This struct defines the way the registers are stored on the
 * kernel stack during a system call or other kernel entry.
 *
 * this should only contain volatile regs
 * since we can keep non-volatile in the tss
 * should set this up when only volatiles are saved
 * by intr code.
 *
 * Since this is going on the stack, *CARE MUST BE TAKEN* to insure
 * that the overall structure is a multiple of 16 bytes in length.
 *
 * Note that the offsets of the fields in this struct correspond with
 * the PT_* values below.  This simplifies arch/ppc/kernel/ptrace.c.
 */

#ifndef __ASSEMBLY__
struct pt_regs {
/* XXX all wrong for 370 */
	unsigned long gpr[16];
	unsigned long nip;
	unsigned long msr;
	unsigned long orig_gpr3; /* Used for restarting system calls */
	unsigned long ctr;
	unsigned long link;
	unsigned long xer;
	unsigned long ccr;
	unsigned long trap;	/* Reason for being here */
	unsigned long dar;	/* Fault registers */
	unsigned long dsisr;
	unsigned long result;   /* Result of a system call */
};
#endif

#define STACK_FRAME_OVERHEAD	16	/* size of minimum stack frame */

/* Size of stack frame allocated when calling signal handler. */
#define __SIGNAL_FRAMESIZE	64

#define instruction_pointer(regs) ((regs)->nip)
#define user_mode(regs) ((regs)->msr & 0x4000)

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

#define PT_NIP	32
#define PT_MSR	33
#ifdef __KERNEL__
#define PT_ORIG_R3 34
#endif
#define PT_CTR	35
#define PT_LNK	36
#define PT_XER	37
#define PT_CCR	38

#define PT_FPR0	48	/* each FP reg occupies 2 slots in this space */
#define PT_FPR31 (PT_FPR0 + 2*31)
#define PT_FPSCR (PT_FPR0 + 2*32 + 1)

#endif

