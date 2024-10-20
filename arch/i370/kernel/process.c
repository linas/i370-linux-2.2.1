/*
 * linux/arch/i370/kernel/process.c
 *
 * Derived from "linux/arch/ppc/kernel/process.c", copyrights apply:
 *   Copyright (C) 1995  Linus Torvalds
 *   Copyright (C) 1995-1996 Gary Thomas (gdt@linuxppc.org
 *   Copyright (C) Cort Dougan (cort@cs.nmt.edu)
 *   Copyright (C) Paul Mackerras (paulus@cs.anu.edu.au)
 *
 * Modified to implement Linux for the ESA/390 class mainframes
 *   Copyright (C) 1999 Linas Vepstas (linas@linas.org)
 *
 * XXX this works in a broken-like way for the ESA/390
 * needs fixin to be really operational...
 * Changes: using psa.Current in lieu of current [ptr003]
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>

#include <asm/asm.h>
#include <asm/current.h>
#include <asm/elf.h>
#include <asm/pgtable.h>
#include <asm/ptrace.h>
#include <asm/uaccess.h>
#include <asm/user.h>

#define __KERNEL_SYSCALLS__ 1
#include <asm/unistd.h>

static struct vm_area_struct init_mmap = INIT_MMAP;
static struct fs_struct init_fs = INIT_FS;
static struct file * init_fd_array[NR_OPEN] = { NULL, };
static struct files_struct init_files = INIT_FILES;
static struct signal_struct init_signals = INIT_SIGNALS;

struct mm_struct init_mm = INIT_MM;

/* Debugging is easier when we can easily find this. So, align it on
   a (two) page boundary. Place it at the begining of the .data section.
   This location is set up in the vmlinux.lds linker script. */
union task_union init_task_union
	// __attribute__((section(".data.init_task"), aligned(2*PAGE_SIZE)))
	// __attribute__((section(".data"), aligned(2*PAGE_SIZE)))
	__attribute__((aligned(2*PAGE_SIZE)))
	= { task: INIT_TASK };

/* init_ksp is used only in head.S during startup to set up the initial stack */
const unsigned long init_ksp __initdata = (unsigned long) init_stack;

/* =================================================================== */

static inline unsigned long
kernel_stack_bot(struct task_struct *tsk)
{
	unsigned long bot = ((unsigned long)tsk) + TASK_STRUCT_SIZE;
	return bot;
}

static inline unsigned long
kernel_stack_top(struct task_struct *tsk)
{
	unsigned long top = ((unsigned long)tsk) + sizeof (union task_union);
	return top;
}

/* =================================================================== */

int dump_fpu(struct pt_regs *regs, elf_fpregset_t *fpregs) {
	printk ("dump fpu \n");
	return 0;
}

void
show_regs(struct pt_regs * regs)
{
	unsigned long cr0, cr1;
	printk("PSW flags: %08lx PSW addr: %08lx ex frame: %p\n",
		regs->psw.flags, regs->psw.addr, regs);
	cr0 =  _stctl_r0();
	cr1 =  _stctl_r1();
	printk (" control regs cr0: %08lx  cr1: %08lx \n", cr0, cr1);

	/* Note: if debugging disabled, r5-r10 may not be valid */
	printk ("  r0: %08lx   r1: %08lx   r2: %08lx   r3: %08lx\n",
		regs->irregs.r0, regs->irregs.r1, regs->irregs.r2, regs->irregs.r3);

	printk ("  r4: %08lx   r5: %08lx   r6: %08lx   r7: %08lx\n",
		regs->irregs.r4, regs->irregs.r5, regs->irregs.r6, regs->irregs.r7);

	printk ("  r8: %08lx   r9: %08lx  r10: %08lx  r11: %08lx\n",
		regs->irregs.r8, regs->irregs.r9, regs->irregs.r10, regs->irregs.r11);

	printk (" r12: %08lx  r13: %08lx  r14: %08lx  r15: %08lx\n",
		regs->irregs.r12, regs->irregs.r13, regs->irregs.r14, regs->irregs.r15);
}

void
print_backtrace (unsigned long stackp)
{
	int cnt = 0;
	i370_elf_stack_t *sp;

	printk("Call Backtrace:\n");

	do
	{
		/* If the stack is at a very high address, assume
		 * that its a user-space stack pointer and that
		 * it needs address translation.
		 */
		if (STACK_TOP - I370_STACK_SIZE <= stackp) {
			unsigned long ksp;
			pte_t *pte = find_pte (current->mm, stackp);
			if (!pte || pte_none(*pte)) {
				make_pages_present (stackp, stackp+4096);
				// make_pages_present (stackp, 0x7fffffff);
				pte = find_pte (current->mm, stackp);
			}
			ksp = pte_page (*pte) | (stackp & ~PAGE_MASK);
			sp = (i370_elf_stack_t *) ksp;
			printk ("   %02d   base[r3]: %lx link[r14]: %lx usp: 0x%lx ksp: %08lx\n",
				cnt, sp->caller_r3, sp->caller_r14, stackp, ksp);

			/* Try walking user stack. Assume it's not insane. */
			stackp = sp->caller_sp;
			if (stackp < STACK_TOP - I370_STACK_SIZE + sizeof(i370_elf_stack_t)) {
				printk ("        ---------------- end of user stack -----------------\n");
				// stackp = current->tss.ksp;
				stackp = _get_SP();
			}
		} else {

			/* We've got a kernel stack pointer now. */
			sp = (i370_elf_stack_t *) stackp;
			printk ("   %02d   base[r3]: %08lx link[r14]: %08lx ksp: %08lx\n",
				cnt, sp->caller_r3, sp->caller_r14, stackp);

			stackp = sp->caller_sp;

			/* Exception handlers intentionally cut the stack. But we want
			   to set the rest of it. We cheat, cause we know where it is */
			if (0 == stackp) {
				i370_interrupt_state_t *exframe = (i370_interrupt_state_t *) sp;
				if (exframe) {
					stackp =  exframe->irregs.r13;
					printk ("        ----------------- exception frame ------------------\n");
				}
			}
		}
		cnt ++;
	} while (stackp &&  (cnt < 10)) ;
	printk("\n");
}

/* =================================================================== */

#define CHECK_STACK
#ifdef CHECK_STACK

/* check to make sure the kernel stack is healthy */
/* check_stack copied raw from from the ppc implementation */
int check_stack(struct task_struct *tsk)
{
	unsigned long stack_bot = kernel_stack_bot(tsk);
	unsigned long stack_top = kernel_stack_top(tsk);
	int ret = 0;

	if (!tsk)
	{
		printk ("check_stack(): bad task %p\n",tsk);
		return 1;
	}

	/* check if stored ksp is bad */
	if ( (tsk->tss.ksp > stack_top) || (tsk->tss.ksp < stack_bot) )
	{
		printk("stack pointer out of bounds: %s/%d\n"
		       "    stack_bot %08lx ksp %08lx stack_top %08lx\n",
		       tsk->comm,tsk->pid,
		       stack_bot, tsk->tss.ksp, stack_top);
		ret |= 2;
	}

	/* check if stack ptr RIGHT NOW is bad */
	if ( (tsk == current) && ((_get_SP() > stack_top ) || (_get_SP() < stack_bot)) )
	{
		printk("current stack ptr out of bounds: %s/%d\n"
		       "    stack_bot %08lx sp %08lx stack_top %08lx\n",
		       current->comm,current->pid,
		       stack_bot, _get_SP(), stack_top);
		ret |= 4;
	}

	/* check if top-of-stack ptr RIGHT NOW is bad */
	if ( (tsk == current) && ((_get_STP() > stack_top ) || (_get_STP() < stack_bot)) )
	{
		printk("current stack top ptr out of bounds: %s/%d\n"
		       "    stack_bot %08lx r11 %08lx stack_top %08lx\n",
		       current->comm,current->pid,
		       stack_bot, _get_STP(), stack_top);
		ret |= 0x8;
	}

	/* check amount of free stack */
	if ( 3000 > (stack_top - tsk->tss.ksp) )
	{
		printk("low on stack space: %s/%d\n"
		       "    stack_bot %08lx ksp %08lx stack_top %08lx\n",
		       tsk->comm,tsk->pid,
		       stack_bot, tsk->tss.ksp, stack_top);
		ret |= 0x10;
	}

	if (ret)
	{
		printk("bad stack, halting\n");
		show_regs (tsk->tss.regs);
		print_backtrace (tsk->tss.regs->irregs.r13);
		i370_halt();
	}
	return(ret);
}
#endif /* CHECK_STACK */

/* =================================================================== */

/*
 * Initialize a thread for running a user-land program
 * This is where:
 * -- we enable DAT (address translation)
 * -- we set the problem state bit in the PSW
 * -- set the user's stack pointer and pass argc, argv, envp
 */
void
i370_start_thread(struct pt_regs *regs, unsigned long nip, unsigned long sp)
{
	cr0_t cr0;

	printk ("i370_start_thread(): setup for user-space thread\n");
	set_fs(USER_DS);

	/* The i370 ELF ABI stack grows up. Thus, the first frame is at
	 * the frame base, which is STACK_TOP - I370_STACK_SIZE.
	 * The I370_STACK_SIZE should be same as _STK_LIM. Currently 8MB.
	 * Per ABI, r11 is the frame pointer. The stack top is just the
	 * frame pointer, plus the size of the frame; which is
	 * sizeof (i370_elf_stack_t) which is 88 bytes.
	 * We'll pass this in r1; this eventually becomes main()'s r13.
	 * To achieve this, _start has to `ST r1,0(,r11)` so main() can
	 * find it.
	 * Pass argc in r2, argv in r3 and envp in r4.
	 */
	unsigned long frame_base = STACK_TOP - I370_STACK_SIZE;

	/* Set up the registers we will hand over to the user. */
	regs->psw.flags &= (PSW_SPACE_MASK | PSW_WAIT);
	regs->psw.flags |= USER_PSW;
	regs->psw.addr = nip | PSW_31BIT;

	// regs->caller_sp = 0;

	/* Setting r15 is pointless; userland will never see this,
	 * because r15 will hold the return value of sys_fork() */
	regs->irregs.r15 = nip | PSW_31BIT;

	/* Don't bother setting r13; userland will figure it out. */
	regs->irregs.r13 = 0;

	/* r11 is the frame pointer. */
	regs->irregs.r11 = frame_base;

	/* r1 gets the stack top */
	regs->irregs.r1 = frame_base + sizeof(i370_elf_stack_t);

	/* r2 gets argc */
	regs->irregs.r2 = sp;

	/* r3 gets argv */
	regs->irregs.r3 = sp + 4;

	/* r4 gets envp */
	/*  __environ = &argv[argc + 1]; */
	unsigned long argc;
	copy_from_user(&argc, (const void *) sp, 4);
	regs->irregs.r4 = sp + 8 + 4*argc;

	/* boffo ace rimer in other regs */
	regs->irregs.r0 = 0xaceb0ff0;
	regs->irregs.r5 = 0xaceb0ff0;
	regs->irregs.r6 = 0xaceb0ff0;
	regs->irregs.r7 = 0xaceb0ff0;
	regs->irregs.r8 = 0xaceb0ff0;
	regs->irregs.r9 = 0xaceb0ff0;
	regs->irregs.r10 = 0xaceb0ff0;
	regs->irregs.r12 = 0xaceb0ff0;
	regs->irregs.r14 = 0xaceb0ff0;

	if (current->tss.regs != regs) {
		printk("i370_start_thread: we really want the correct task!\n");
		show_regs(regs);
		print_backtrace(_get_SP());
		i370_halt();
	}

	/* Enable address translation (virtual memory) for user-space.  */
	cr0.raw = _stctl_r0();
	cr0.bits.tf = 0x16; /* Translation Format. Only valid value. */
	current->tss.cr0 = cr0;
	/* Value in cr0 does not take effect until PSW enables translation. */
	_lctl_r0(cr0.raw);
}

/* =================================================================== */

// switch_to does the task switching.
// I'm not to clear on what it needs to do ...
// ... needs to switch the stack pointer, at least ...
//     if we just switch the stack pointer, then I don't think we need to
//     save/restore any registers, since merely returning from this routine
//     will accomplish all the sve/restore we need.
// ... change addressing modes (potentially, from real
//     to primary or v.v.  ... then again, we can defer changing addressing
//     until we return to the user-level process. Ditto for the next two
//     steps ...
// ... modify the segment table CR1 and maybe CR3 key  ...
// ... Purge TLB ...
//
// NB the first time through, switch_to() actually returns as if
// it were do_fork() to anyone who had called do_fork(), (even
// though switch_to() was called in schedule()).  This is because when
// the stack unwinds during the subr return, it unwinds into a copy
// of it's parent's stack (minus one stackframe).  And the last thing
// the parent had done was a fork, ergo, that's were we return to.
//
// A non-zero return value indicates an error.

// #define SHOW_TASK_SWITCHES
// #define DEBUG
#ifdef DEBUG
	#define DBGPRT(...) printk(__VA_ARGS__)
#else
	#define DBGPRT(...)
#endif

void
i370_switch_to(struct task_struct *prev, struct task_struct *next)
{
	struct thread_struct *new_tss, *old_tss;
	unsigned long thunk;
	__cli ();

#ifdef CHECK_STACK
	check_stack(prev);
	check_stack(next);
#endif /* CHECK_STACK */

#ifdef SHOW_TASK_SWITCHES
	printk("task switch ");
	if (next->tss.regs)
		printk("%s/%d -> %s/%d PSW 0x%lx 0x%lx cpu %d \n",
		       prev->comm,prev->pid,
		       next->comm,next->pid,
		       next->tss.regs->psw.flags,
		       next->tss.regs->psw.addr,
		       next->processor);
	else
		printk("%s/%d -> %s/%d Error: NULL next regs!!\n",
		       prev->comm,prev->pid,
		       next->comm,next->pid);

	printk("current regs=%p prev=%p next=%p\n",
	        current->tss.regs, prev->tss.regs, next->tss.regs);

	// printk ("current sp=0x%lx next sp=0x%lx\n", _get_SP(), next->tss.ksp);

	/* Sometimes the PSW in the regs is NULL, and this is associated
	 * with the swapper threads clone from pid 0. But it seems to be
	 * totally harmless, so ... I'm gonna ignore it. */
	if (!next->tss.regs)
		printk ("Warning: NULL regs for next pid=%d\n", next->pid);
	else if (!next->tss.regs->psw.addr)
		printk ("Warning: NULL PSW for next pid=%d\n", next->pid);

	if (!prev->tss.regs)
		printk ("Warning: NULL regs for prev pid=%d\n", prev->pid);
	else if (!prev->tss.regs->psw.addr)
		printk ("Warning: NULL PSW for prev pid=%d\n", prev->pid);
#endif

#ifdef __SMP__
	prev->last_processor = prev->processor;
	/* XXX copy  current=new to the pfx page of the correct processor. */
#endif /* __SMP__ */

	old_tss = &prev->tss;
	new_tss = &next->tss;

	/* Save and restore flt point regs.  We do this here because
	 * it is NOT done at return from interrupt (in order to save
	 * some overhead). Kernel doesn't use fpt,s so we only need to
	 * do this if coming rom, going to user-land. */
	_store_fpregs (old_tss->fpr);
	_load_fpregs (new_tss->fpr);

	current = next;

	/* Switch control registers.
	   cr0 enables/disables address translation, and
	   cr1 contains the segment table origin.
	   The new values do not take effect until the PSW is set.
	 */
	old_tss->cr0.raw = _stctl_r0();
	old_tss->cr1.raw = _stctl_r1();
	_lctl_r1 (new_tss->cr1.raw);
	_lctl_r0 (new_tss->cr0.raw);

	/* Switch kernel stack pointers.  */
	old_tss->ksp = _get_SP();
	thunk = _get_STP() - _get_SP();
	_set_STP (new_tss->ksp + thunk);
	_set_SP (new_tss->ksp);

	/* Purge TLB; the segment table origin changed in cr1.  */
	_ptlb();

	/* Enable interrupts.  */
	__sti ();
}

/* =================================================================== */

void exit_thread(void)
{
	/* The only thing we have to do here is to check if a userland
	 * thread has used the float pt regs, and maybe make sure we
	 * want to clobber/save or restore these. But right now, we
	 * unconditionally save/restore float pt regs in swtich_to()
	 * so this doesn't matter. */
	DBGPRT("Call exit_thread\n");
}

void flush_thread(void)
{
	/* The only thing we have to do here is to check if a userland
	 * thread has used the float pt regs, and maybe make sure we
	 * want to clobber/save or restore these. But right now, we
	 * unconditionally save/restore float pt regs in swtich_to()
	 * so this doesn't matter. */
}

void
release_thread(struct task_struct *t)
{
	DBGPRT("Call release_thread\n");
}

/* =================================================================== */

/* Copy the standard i370 elf stack, thunking the stack and frame
   pointers in each stackframe. This would all be much easier if
   the stack did not actually contain any stack or frame pointers,
   but only the frame size. However, the i370 elf stack cloned the
   basic MVS stack design, which does store such things.
 */
int copy_user_stack(unsigned long dstsp, unsigned long srcsp,
                    unsigned long delta)
{
	unsigned long or11, or13;
	unsigned long bot, top;
	unsigned long frsize;
#define MAXUFRSZ 120
	unsigned long frame[MAXUFRSZ];

	/* Offset to frame and stack pointers in standard elf frame.  */
	or11 = ((unsigned long) &((i370_elf_stack_t*)0)->callee_sp) >> 2;
	or13 = ((unsigned long) &((i370_elf_stack_t*)0)->caller_sp) >> 2;

	/* Standard frame size. We assume this for the top-most frame,
	   which should be the fork() frame which has no args to copy.  */
	frsize = sizeof(i370_elf_stack_t);

	/* Walk down the stack, thunk the pointers. */
	do {
		DBGPRT("Copy user stackframe from %lx to %lx size 0x%lx\n",
		        srcsp, dstsp, frsize);

		/* Copy in one stackframe. */
		if (!access_ok(VERIFY_READ, (void *) srcsp, frsize))
			return -EFAULT;
		__copy_from_user(frame, (void *) srcsp, frsize);

		bot = frame[or13];
		top = frame[or11];

		/* Sanity check.  */
		if (srcsp != top)
			return -EFAULT;

		/* Thunk the pointers. */
		frame[or13] -= delta;
		frame[or11] -= delta;

		if (!access_ok(VERIFY_WRITE, (void *) dstsp, frsize))
			return -EFAULT;

		__copy_to_user(dstsp, (void *) frame, frsize);

		frsize = srcsp - bot;
		srcsp = bot;
		dstsp = bot - delta;

		if (4*MAXUFRSZ < frsize)
			return -EFAULT;

		/* Last frame points at zero, or itself. */
	} while (frsize && srcsp);

	DBGPRT("Done copying user regs\n");

	return 0;
}

/* =================================================================== */

/*
 * Copy a thread.
 *
 * -- Copy i370 parts of the task structure (struct thread_struct).
 * -- Thunk stack points in normal frames and exception frames.
 * -- If coming from user, set up the user stack pointer
 * -- Load CR1 with page table origin ??!! (Not done??)
 * -- Copy PSW flags (in particular, the DAT flag) ???
 *
 * The copy is from 'current' to mem location at 'p'.
 * usp and regs are exactly what was passed to do_fork.
 * It is assumed that usp is the stack pointer before
 * the svc took place.
 *
 * Return 0 if success, errno if not.
 */

int
copy_thread(int nr, unsigned long clone_flags, unsigned long usp,
            struct task_struct * p, struct pt_regs * regs)
{
	unsigned long srctop, dsttop;
	i370_elf_stack_t *srcsp, *dstsp, *lastdst;
	i370_elf_stack_t *this_frame, *this_frame_top;
	void *srcbase, *dstbase;
	unsigned long delta;
	i370_interrupt_state_t *srcregs, **dstregs;
	int rc;

	DBGPRT("i370_copy_thread, %s/%d regs=%p usp=0x%lx\n",
	        current->comm, current->pid, current->tss.regs, usp);

	/* Copy the kernel stack, and thunk the stack pointers in it.
	 * Don't copy the current frame: we want the new stack to
	 * unwind as if it were returning from do_fork(), instead of
	 * returning to do_fork(). The value of the thunk is 'delta'.
	 */
	srctop = kernel_stack_top (current);
	dsttop = kernel_stack_top (p);
	delta = srctop - dsttop;

	this_frame = (i370_elf_stack_t *) _get_SP();
	this_frame_top = this_frame;
	this_frame = (i370_elf_stack_t *) (this_frame->caller_sp);
	srcsp = this_frame;

	/* switch_to() grabs the current ksp out of tss.ksp */
	p->tss.ksp = ((unsigned long) this_frame) - delta;
	DBGPRT("i370_copy_thread srctop=%lx frtop=%p frame=%p delta=%lx\n",
	       srctop, this_frame_top, this_frame, -delta);

#ifdef CHECK_STACK
	check_stack(current);
	check_stack(p);
#endif /* CHECK_STACK */

	srcbase = (void *) kernel_stack_bot (current);
	dstbase = (void *) kernel_stack_bot (p);
	memcpy (dstbase, srcbase, kernel_stack_top(p)-kernel_stack_bot(p));
	dstsp = (i370_elf_stack_t *) (((unsigned long) srcsp) - delta);
	do {
		dstsp -> caller_sp = srcsp->caller_sp - delta;
		dstsp -> callee_sp = srcsp->callee_sp - delta;
		srcsp = (i370_elf_stack_t *) (srcsp -> caller_sp);
		dstsp = (i370_elf_stack_t *) (dstsp -> caller_sp);
	} while (srcsp && ((unsigned long)regs < (unsigned long)srcsp));

	/* Hang on... I thought there was one more, the exception frame ??? */
	/* But now there isn't? I'm confused. I thought I fixed this once.  */
#if 0
	/* The last frame is an exception frame. */
	dstsp -> caller_sp = srcsp->caller_sp - delta;
	dstsp = (i370_elf_stack_t *) (dstsp -> caller_sp);
#else
	if (0 != srcsp->caller_sp) {
		printk("copy_thread: unexpected unterminated stack\n");
		show_regs(current->tss.regs);
		print_backtrace(_get_SP());
		i370_halt();
	}
#endif
	dstsp -> caller_sp = 0;

	DBGPRT("i370_copy_thread done copying stack\n");

	/* An exception frame (_i370_interrupt_state_t) is stored on stack.
	 * That frame holds r13 and r11 that were put there by the SVC
	 * EXCEPTION_PROLOG (in head.S) and the EPILOG will restore them
	 * when the SVC returns.  We have to find these values and thunk
	 * them.  The code below will correctly handle nested interrupts,
	 * although in theory we should never have more than two (?)
	 * interrupts on the kernel stack (one for fork, followed by one
	 * for clone.)
	 */
	srcregs = current->tss.regs;
	dstregs = &(p->tss.regs);

	if (!srcregs) {
		/* This can't happen, but it did ... Last time it happened,
		 * its because the #define _psa_current was wrong and head.S
		 * didn't pick up the right value.  Unfortunately, this is
		 * likely to happen again.
		 */
		printk("i370_copy_thread, damaged regs pointer\n");
		printk("Please check offset _psa_current psa.h and in head.S\n");
		show_regs (regs);
		print_backtrace ((unsigned long) this_frame);
		i370_halt();
	}

	/* Note: oldregs and psw are set in head.S by the exception prolog. */
	do {
		*dstregs = (i370_interrupt_state_t *)
			(((unsigned long) srcregs) - delta);
		DBGPRT("Thunk dstregs %p from src %p\n", *dstregs, srcregs);
		(*dstregs)->caller_sp = 0;
		(*dstregs)->psw = srcregs->psw;
		(*dstregs)->irregs.r13 = srcregs->irregs.r13 - delta;
		(*dstregs)->irregs.r11 = srcregs->irregs.r11 - delta;
		(*dstregs)->oldregs =  (i370_interrupt_state_t *)
			(((unsigned long) (srcregs->oldregs)) - delta);
		srcregs = srcregs->oldregs;
		dstregs = &((*dstregs)->oldregs);
	} while (srcregs);
	*dstregs = (i370_interrupt_state_t *) 0;
	DBGPRT("i370_copy_thread done copying regs\n");

	/* Now copy the 'far side' of the stack, the part that lies
	 * on the other side of the SVC that got us here.  There are
	 * two possibilities here: that we came from user-land,
	 * or that we came from the kernel. If we came from the
	 * kernel, then we are still on the same stack. Copy that, too.
	 */
	if (regs->psw.flags & PSW_PROB) {
		rc = copy_user_stack(usp-delta, usp, delta);
		if (rc < 0) return rc;
#ifdef DEBUG
		DBGPRT("i370_copy_thread return to user with child regs\n");
		show_regs(p->tss.regs);
#endif
	} else {
		/* Validate expected location of the stack. */
		if ((usp > (unsigned long) srcsp)) {
			printk("i370_copy_thread, bad usp=0x%lx, "
				"lastsrc=%p, stcktop=0x%lx \n",
				usp, srcsp, srctop);
			i370_halt();
		}

		/* Same kernel stack, just fix up entries. */
		srcsp = (i370_elf_stack_t *) usp;
		dstsp = (i370_elf_stack_t *) (usp - delta);
		do {
			dstsp -> caller_sp = srcsp->caller_sp - delta;
			dstsp -> callee_sp = srcsp->callee_sp - delta;
			lastdst = dstsp;
			srcsp = (i370_elf_stack_t *) (srcsp -> caller_sp);
			dstsp = (i370_elf_stack_t *) (dstsp -> caller_sp);
		} while (srcsp);
		lastdst -> caller_sp = 0;
	}

	DBGPRT ("i370_copy_thread: finished %s/%d regs=%p\n",
	        p->comm, p->pid, p->tss.regs);
#ifdef __SMP__
	if ( (p->pid != 0) || !(clone_flags & CLONE_PID) )
		p->tss.smp_fork_ret = 1;
	p->last_processor = NO_PROC_ID;
#endif /* __SMP__ */

	return 0;
}

/* =================================================================== */
/*
 * do_fork will call copy_thread, passing sp and regs to it.
 */
asmlinkage int
i370_sys_fork (void)
{
	struct pt_regs *regs;
	int res = 0;

	lock_kernel();
	regs = current->tss.regs;
	res = do_fork(SIGCHLD, regs->irregs.r13, regs);
	_lctl_r1(current->tss.cr1.raw);

#ifdef __SMP__
	/* When we clone the idle task we keep the same pid but
	 * the return value of 0 for both causes problems.
	 * -- Cort
	 */
	if ((current->pid == 0) && (current == &init_task))
		res = 1;
#endif /* __SMP__ */
	unlock_kernel();
	return res;
}

/* =================================================================== */

asmlinkage int
i370_sys_clone (unsigned long clone_flags)
{
	struct pt_regs *regs;
	int res = 0;

	DBGPRT ("i370_sys_clone of %s/%d regs=%p flags=0x%lx ksp=%lx sp=%lx\n",
	        current->comm, current->pid, current->tss.regs,
	        clone_flags, current->tss.ksp, _get_SP());

	lock_kernel();
	regs = current->tss.regs;

	/* All args pass to copy_thread.  */
	res = do_fork(clone_flags, regs->irregs.r13, regs);
	_lctl_r1(current->tss.cr1.raw);

	if ((res < 0) && (0 == user_mode(current->tss.regs))) {
		printk("Error: i370_sys_clone failed for %s/%d\n",
		        current->comm, current->pid);
		show_regs(regs);
		print_backtrace(_get_SP());
		i370_halt();
	}

#ifdef __SMP__
	/* When we clone the idle task we keep the same pid but
	 * the return value of 0 for both causes problems.
	 */
	if ((current->pid == 0) && (current == &init_task))
		res = 1;
#endif /* __SMP__ */
	unlock_kernel();

	DBGPRT ("i370_sys_clone(): after do_fork, %s/%d regs=%p res=%d\n",
	        current->comm, current->pid, current->tss.regs, res);

	return res;
}

/* =================================================================== */
/*
 * i370_kernel_thread ... creates a new kernel thread.
 * Called during initialization, and during module load to create
 * misc important kernel threads.
 *
 * The child is supposed to then call the arg fn(), and never return.
 * If for any reason fn() returns, the child thread is supposed to be
 * cleaned up and discarded.
 *
 * clone() is a system call, it results in i370_sys_clone() being
 * called.  In turn, i370_sys_clone just calls do_fork() which then
 * calls copy_thread().  We can't call do_fork directly, we must
 * svc into it in order to be able to schedule and return properly
 * in the child process.
 */

long
i370_kernel_thread(unsigned long flags, int (*fn)(void *), void *args)
{
	long pid;
	DBGPRT ("i370_kernel_thread %s/%d regs=%p ksp=%lx sp=%lx\n",
	        current->comm, current->pid, current->tss.regs,
	        current->tss.ksp, _get_SP());

	/* Record the control registers. We'll need them later.  */
	current->tss.cr0.raw = _stctl_r0();
	current->tss.cr1.raw = _stctl_r1();

	pid = clone (flags); /* Goes through SVC */
	DBGPRT ("i370_kernel_thread(): return from clone, new pid=%ld\n",pid);
	DBGPRT ("i370_kernel_thread(): I still am %s/%d regs=%p\n",
	        current->comm, current->pid, current->tss.regs);
	if (pid) return pid;
	fn (args);
	printk ("i370_kernel_thread(): Unexpected return from fn\n");
	while (1) {_exit (1); }
	return 0;
}

/* =================================================================== */
/*
 * Fill in the user structure for a core dump.
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
	/* Used only for MS-DOS binaries */
	memset(dump, 0, sizeof(struct user));
}

/* ========================== END OF FILE =========================== */
