
/* Signal handling. Stubbed in. Details are probably wrong. */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/current.h>
#include <asm/signal.h>
#include <asm/spinlock.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>

extern int sys_wait4(int, int *, int, struct rusage *);

int i370_sys_sigaction (int sig,
                        const struct old_sigaction *act,
                        const struct old_sigaction *oact)
{
	printk("i370_sys_sigaction: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 1;
}

int i370_sys_sigsuspend (old_sigset_t mask, struct pt_regs *regs)
{
	printk("i370_sys_sigsuspend: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 1;
}

int i370_sys_rt_sigsuspend (sigset_t *unewset, size_t sigsetsize,
                            struct pt_regs *regs)
{
	printk("i370_sys_rt_sigsuspend: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 1;
}

int i370_sys_sigreturn (struct pt_regs *regs)
{
	printk("i370_sys_sigreturn: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 1;
}

int i370_sys_rt_sigreturn (void)
{
	printk("i370_sys_rt_sigreturn: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 1;
}

int i370_sys_sigaltstack (const stack_t *uss, stack_t *uoss)
{
	printk("i370_sys_sigalstack: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 1;
}

/*
 * Signals are delivered on stack. For ease of use, the stack
 * layout is defined by struct sigframe below.
 */
struct sigframe {
	i370_elf_stack_t elf_abi_frame;
	unsigned long args; /* only one, the signo. At 88(r11) */
	unsigned char  tramp[8];

	i370_interrupt_state_t istate;
	// struct sigcontext scc; /* todo */
};

/*
 * Set up the return.
 *    LA   r1,__NR_sigreturn(0,0)
 *    SVC  0
 */
static void inline
setup_trampoline(unsigned char *code)
{
	*((unsigned long *) code) = 0x41100000 + __NR_sigreturn; /* LA */
	code += 4;
	*((unsigned short *) code) = 0x0a00; /* SVC */
}

/*
 * Set up a signal frame on the user's stack.
 * Return to the user's handler.
 */
static unsigned long
setup_frame(unsigned long signo, struct k_sigaction *ka, struct pt_regs *regs)
{
	unsigned long uhand_addr;
	unsigned long frbottom;
	unsigned long frtop;
	struct sigframe sf, *sfp;

	uhand_addr = (unsigned long) ka->sa.sa_handler;
	uhand_addr |= 0x80000000;

	printk("Setup signal frame. signr=%ld handler=%lx\n", signo, uhand_addr);

	/* First check if we can even do this. */
	frbottom = regs->irregs.r11;
	frtop = frbottom + sizeof(struct sigframe);
	sfp = (struct sigframe *) frbottom;
	if (verify_area(VERIFY_WRITE, sfp, sizeof(struct sigframe)))
		goto badframe;

	/* Now set it up. */
	memset(&sf, 0, sizeof(struct sigframe));

	/* Save our interrupt state; well need it later. */
	sf.istate = *regs;

	/* Zap our irregs to avoid future confusion */
	memset(&regs->irregs, 0, sizeof(irregs_t));

	/* Increment the users stack pointer by the size of our frame. */
	regs->irregs.r11 = frtop;

	// regs->caller_sp = ???

	/* Place the signr where user can find it. Should be 88(r11) */
	sf.args = signo;

	/* EXCEPTION_EPILOG will return to PSW */
	regs->psw.addr = uhand_addr;

	/* Pretend the handler was invoked as BASR 14,15 */
	/* Except this won't work, because SVC EPILOG doesn't do it. */
	regs->irregs.r15 = uhand_addr;

	/* Set up the return from the users handler. User will
	 * BASR r1,r14 so r14 better point at somethng good. */
	setup_trampoline(&sf.tramp[0]);
	regs->irregs.r14 = frbottom +
		(unsigned long) &sf.tramp - (unsigned long) &sf;

	/* Copy the hot mess to the users stack. */
	if (__copy_to_user(sfp, &sf, sizeof(struct sigframe)))
		goto badframe;

	return uhand_addr;

badframe:
	printk("badframe in setup_frame\n");
	show_regs(regs);

	lock_kernel();
	do_exit(SIGSEGV);
}

static unsigned long
setup_rt_frame(unsigned long signo, struct k_sigaction *ka,
               siginfo_t *info, struct pt_regs *regs)
{
	printk("setup_rt_frame: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 0;
}

/*
 * OK, we're invoking a handler
 */
static unsigned long
handle_signal(unsigned long sig, struct k_sigaction *ka,
              siginfo_t *info, sigset_t *oldset, struct pt_regs * regs)
{
	unsigned long rc;
	/* XXX TODO if system call, and not ka->sa.sa_flags & SA_RESTART
	 * then irregs->r15 = -EINTR;
	 */

	if (ka->sa.sa_flags & SA_SIGINFO)
		rc = setup_rt_frame(sig, ka, info, regs);
	else
		rc = setup_frame(sig, ka, regs);

	if (ka->sa.sa_flags & SA_ONESHOT)
		ka->sa.sa_handler = SIG_DFL;
	if (!(ka->sa.sa_flags & SA_NODEFER)) {
		spin_lock_irq(&current->sigmask_lock);
		sigorsets(&current->blocked,&current->blocked,&ka->sa.sa_mask);
		sigaddset(&current->blocked,sig);
		recalc_sigpending(current);
		spin_unlock_irq(&current->sigmask_lock);
	}

	return rc;
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
unsigned long
i370_do_signal(sigset_t *oldset, struct pt_regs *regs)
{
	siginfo_t info;
	struct k_sigaction *ka;

	if (!oldset)
		oldset = &current->blocked;

	printk("i370_do_signal %s/%d oldset=%p\n",
	       current->comm, current->pid, oldset);

	while (1) {
		unsigned long signr;

		spin_lock_irq(&current->sigmask_lock);
		signr = dequeue_signal(&current->blocked, &info);
		spin_unlock_irq(&current->sigmask_lock);

		if (!signr)
			break;

		if ((current->flags & PF_PTRACED) && signr != SIGKILL) {
			/* Let the debugger run.  */
			current->exit_code = signr;
			current->state = TASK_STOPPED;
			notify_parent(current, SIGCHLD);
			schedule();

			/* We're back.  Did the debugger cancel the sig?  */
			if (!(signr = current->exit_code))
				continue;
			current->exit_code = 0;

			/* The debugger continued.  Ignore SIGSTOP.  */
			if (signr == SIGSTOP)
				continue;

			/* Update the siginfo structure.  Is this good?  */
			if (signr != info.si_signo) {
				info.si_signo = signr;
				info.si_errno = 0;
				info.si_code = SI_USER;
				info.si_pid = current->p_pptr->pid;
				info.si_uid = current->p_pptr->uid;
			}

			/* If the (new) signal is now blocked, requeue it.  */
			if (sigismember(&current->blocked, signr)) {
				send_sig_info(signr, &info, current);
				continue;
			}
		}

		ka = &current->sig->action[signr-1];
		if (ka->sa.sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* Check for SIGCHLD: it's special.  */
			while (sys_wait4(-1, NULL, WNOHANG, NULL) > 0) {}
			continue;
		}
		if (ka->sa.sa_handler == SIG_DFL) {
			int exit_code = signr;

			/* Init gets no signals it doesn't want.  */
			if (current->pid == 1)
				continue;

			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp(current->pgrp))
					continue;
				/* FALLTHRU */

			case SIGSTOP:
				current->state = TASK_STOPPED;
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa.sa_flags & SA_NOCLDSTOP))
					notify_parent(current, SIGCHLD);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGABRT: case SIGFPE: case SIGSEGV:
				lock_kernel();
				if (current->binfmt
					 && current->binfmt->core_dump
					 && current->binfmt->core_dump(signr, regs))
					exit_code |= 0x80;
				unlock_kernel();
				/* FALLTHRU */

			default:
				lock_kernel();
				sigaddset(&current->signal, signr);
				current->flags |= PF_SIGNALED;
				do_exit(exit_code);
				/* NOTREACHED */
			}
		}

#if 0
		/* Something something. If ERESTARTNOHAND ERESTARTSYS
		 * ERESTARTNOINTR change it to EINTR and do it again.
		 * See arch/alpha/kernel/signal.c for example. */
		if (regs->r15)
			syscall_restart(regs, ka);
#endif

		/* Whee!  Actually deliver the signal.  */
		return handle_signal(signr, ka, &info, oldset, regs);
	}

	/* TODO check for ERESTARTNOHAND ERESTARTSYS ERESTARTNOINTR
	 * back up to last insn which should be SVC and restart.
	 * See arch/alpha/kernel/signal.c for example.
	 */

	return 0;      /* no signals delivered */
}
