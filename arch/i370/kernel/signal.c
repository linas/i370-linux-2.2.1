
/* XXX signal.c not implemented. Take a look at ppc/signal.h for ideas. */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/current.h>
#include <asm/signal.h>
#include <asm/spinlock.h>
#include <asm/system.h>

extern int sys_wait4(int, int *, int, struct rusage *);

/*
 * OK, we're invoking a handler
 */
static void
handle_signal(unsigned long sig, struct k_sigaction *ka,
              siginfo_t *info, sigset_t *oldset, struct pt_regs * regs,
              unsigned long *newspp, unsigned long frame)
{
printk("XXX handle_signa for signals not implemented yet\n");
}

/*
 * Set up a signal frame.
 */
static void
setup_frame(struct pt_regs *regs, struct sigregs *frame,
            unsigned long newsp)
{
	struct sigcontext_struct *sc = (struct sigcontext_struct *) newsp;
	if (verify_area(VERIFY_WRITE, frame, sizeof(*frame)))
		goto badframe;

	printk("Trying to setup signal context frame. Probably did it wrong\n");

	if (__copy_to_user(&frame->gp_regs, regs, sizeof(struct pt_regs)))
		goto badframe;
	/* XXX TODO copy fpregs, too */

	/* XXX FIXME. I have no idea. We have to jump to somewhere,
	 * not sure where. */
	if (__put_user(0x0def0000UL, &frame->tramp[0])) /* basr 14,15 */
		goto badframe;

	newsp -= __SIGNAL_FRAMESIZE;

	if (put_user(regs->r13, newsp))
		goto badframe;
	if (get_user(regs->irregs.r15, &sc->handler))
		goto badframe;
	/* XXX also get_user of &sc->signal to somewhere on stack. */

	/* XXX FIXME. Wild guess. This is approximately correct but probably
	 * wrong. */
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

	newsp = regs->irregs.r13;
	frame = regs->irregs.r11;

printk("Debug: enter i370_do_signal oldset=%x sp=%x fr=%s\n", oldset, newsp, frame);
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

		/* Whee!  Actually deliver the signal.  */
		handle_signal(signr, ka, &info, oldset, regs, &newsp, frame);
	}

	if (newsp == frame)
		return 0;      /* no signals delivered */

	setup_frame(regs, (struct sigregs *) frame, newsp);
	return 1;
}
