
/* XXX signal.c not implemented. Take a look at ppc/signal.h for ideas. */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <asm/current.h>
#include <asm/signal.h>
#include <asm/spinlock.h>
#include <asm/system.h>

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 */
int
i370_do_signal(sigset_t *oldset, struct pt_regs *regs)
{
	siginfo_t info;

	if (!oldset)
		oldset = &current->blocked;

	// newsp = frame = regs->gpr[1] - sizeof(struct sigregs);
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

	}

	printk ("called do_signal XXX not implemented \n");
	return 0;
}
