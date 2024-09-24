#ifndef _ASM_I370_SIGCONTEXT_H
#define _ASM_I370_SIGCONTEXT_H

#include <asm/ptrace.h>


struct sigcontext_struct {
	unsigned long	_unused[4];
	int		signal;
	unsigned long	handler;
	unsigned long	oldmask;
	struct pt_regs 	*regs;
};

int i370_do_signal(sigset_t *oldset, struct pt_regs *regs);

#endif
