
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
	i370_interrupt_state_t pstate; /* same as struct pt_regs */
	unsigned char  tramp[8];
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
	put_user_data(0x41100000 + __NR_sigreturn, code    , 4); /* LA */
	put_user_data(0x0a00,                      code + 4, 2); /* SVC */
}

/*
 * Set up a signal frame.
 */
static void
setup_frame(struct pt_regs *regs, struct sigframe* frame,
            unsigned long newsp)
{
	struct sigcontext_struct *sc = (struct sigcontext_struct *) newsp;
	if (verify_area(VERIFY_WRITE, frame, sizeof(*frame)))
		goto badframe;

	printk("Trying to setup signal context frame. Probably did it wrong\n");

	if (__copy_to_user(&frame->pstate, regs, sizeof(struct pt_regs)))
		goto badframe;
	/* XXX TODO copy fpregs, too */

	setup_trampoline(&frame->tramp[0]);

	newsp += sizeof(struct sigframe);

	/* XXX FIXME this is probably insane */
	if (put_user(regs->irregs.r11, (unsigned long*) newsp))
		goto badframe;
	if (get_user(regs->irregs.r15, &sc->handler))
		goto badframe;
	/* XXX also get_user of &sc->signal to somewhere on stack. */

	printk("Setting up signal context frame. frame=%p sp=0x%lx\n",
	       frame, newsp);

	/* XXX FIXME. Wild guess. I probably did this wrong. */
	regs->irregs.r13 = newsp;
	regs->irregs.r11 = (unsigned long) sc;
	regs->irregs.r15 = (unsigned long) frame->tramp;

	return;

badframe:
	printk("badframe in setup_frame, regs=%p frame=%p newsp=%lx\n",
	       regs, frame, newsp);

	lock_kernel();
	do_exit(SIGSEGV);
}

static void
setup_rt_frame(struct pt_regs *regs, struct sigframe* frame,
               unsigned long newsp, siginfo_t *info)
{
	printk("setup_rt_frame: not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
}

/*
 * OK, we're invoking a handler
 */
static void
handle_signal(unsigned long sig, struct k_sigaction *ka,
              siginfo_t *info, sigset_t *oldset, struct pt_regs * regs,
              unsigned long newsp, unsigned long frame)
{
	/* XXX TODO if system call, and not ka->sa.sa_flags & SA_RESTART
	 * then irregs->r15 = -EINTR;
	 */

	if (ka->sa.sa_flags & SA_SIGINFO)
		setup_rt_frame(regs, (struct sigframe *) frame, newsp, info);
	else
		setup_frame(regs, (struct sigframe *) frame, newsp);

	if (ka->sa.sa_flags & SA_ONESHOT)
		ka->sa.sa_handler = SIG_DFL;
	if (!(ka->sa.sa_flags & SA_NODEFER)) {
		spin_lock_irq(&current->sigmask_lock);
		sigorsets(&current->blocked,&current->blocked,&ka->sa.sa_mask);
		sigaddset(&current->blocked,sig);
		recalc_sigpending(current);
		spin_unlock_irq(&current->sigmask_lock);
	}
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
int
i370_do_signal(sigset_t *oldset, struct pt_regs *regs)
{
	siginfo_t info;
	struct k_sigaction *ka;
	unsigned long frame, newsp;

	if (!oldset)
		oldset = &current->blocked;

	frame = regs->irregs.r13;
	newsp = regs->irregs.r11;

	printk("i370_do_signal %s/%d oldset=%p sp=%lx fr=%lx\n",
	       current->comm, current->pid, oldset, newsp, frame);

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
			while (sys_wait4(-1, NULL, WNOHANG, NULL) > 0)
				/* nothing */;
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
		handle_signal(signr, ka, &info, oldset, regs, newsp, frame);
		return 1;
	}

	/* TODO check for ERESTARTNOHAND ERESTARTSYS ERESTARTNOINTR
	 * back up to last insn which should be SVC and restart.
	 * See arch/alpha/kernel/signal.c for example.
	 */

	return 0;      /* no signals delivered */
}
