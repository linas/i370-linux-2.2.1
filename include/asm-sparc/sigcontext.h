/* $Id: sigcontext.h,v 1.1.1.1 1999/02/08 06:18:59 linas Exp $ */
#ifndef __SPARC_SIGCONTEXT_H
#define __SPARC_SIGCONTEXT_H

#include <asm/ptrace.h>

#define SUNOS_MAXWIN   31

#ifndef __ASSEMBLY__

/* SunOS system call sigstack() uses this arg. */
struct sunos_sigstack {
	unsigned long sig_sp;
	int onstack_flag;
};

/* This is what SunOS does, so shall I. */
struct sigcontext {
	int sigc_onstack;      /* state to restore */
	int sigc_mask;         /* sigmask to restore */
	int sigc_sp;           /* stack pointer */
	int sigc_pc;           /* program counter */
	int sigc_npc;          /* next program counter */
	int sigc_psr;          /* for condition codes etc */
	int sigc_g1;           /* User uses these two registers */
	int sigc_o0;           /* within the trampoline code. */

	/* Now comes information regarding the users window set
	 * at the time of the signal.
	 */
	int sigc_oswins;       /* outstanding windows */

	/* stack ptrs for each regwin buf */
	char *sigc_spbuf[SUNOS_MAXWIN];

	/* Windows to restore after signal */
	struct reg_window sigc_wbuf[SUNOS_MAXWIN];
};

typedef struct {
	struct pt_regs	si_regs;
	int		si_mask;
} __siginfo_t;

typedef struct {
	unsigned   long si_float_regs [32];
	unsigned   long si_fsr;
	unsigned   long si_fpqdepth;
	struct {
		unsigned long *insn_addr;
		unsigned long insn;
	} si_fpqueue [16];
} __siginfo_fpu_t;

/* This magic should be in g_upper[0] for all upper parts
   to be valid.
   This is generated by sparc64 only, but for 32bit processes,
   so we define it here as well.  */
#define SIGINFO_EXTRA_V8PLUS_MAGIC      0x130e269
typedef struct {
	unsigned   int g_upper[8];
	unsigned   int o_upper[8];
} siginfo_extra_v8plus_t;

#endif /* !(__ASSEMBLY__) */

#endif /* !(__SPARC_SIGCONTEXT_H) */
