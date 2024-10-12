#ifndef _ASMI370_UCONTEXT_H
#define _ASMI370_UCONTEXT_H

/* Copied from i386. */

struct ucontext {
	unsigned long	  uc_flags;
	struct ucontext  *uc_link;
	stack_t		  uc_stack;
	sigset_t	  uc_sigmask;	/* mask last for extensibility */
};

#endif /* !_ASMI370_UCONTEXT_H */
