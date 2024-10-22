/*
 *  linux/arch/i370/kernel/traps.c
 *  copied from the ppc implementation; ppc copyrights apply to this code.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */

/*
 * This file handles the architecture-dependent parts of hardware exceptions
 */


#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/sched.h>

#include <asm/asm.h>
#include <asm/current.h>
#include <asm/iorb.h>
#include <asm/irq.h>
#include <asm/param.h>
#include <asm/ptrace.h>
#include <asm/signal.h>
#include <asm/system.h>
#include <asm/trace.h>
#include <asm/unitblk.h>


#ifdef CONFIG_XMON
extern void xmon(struct pt_regs *regs);
extern int xmon_bpt(struct pt_regs *regs);
extern int xmon_sstep(struct pt_regs *regs);
extern int xmon_iabr_match(struct pt_regs *regs);
extern int xmon_dabr_match(struct pt_regs *regs);
extern void (*xmon_fault_handler)(struct pt_regs *regs);
#endif

#ifdef CONFIG_XMON
void (*debugger)(struct pt_regs *regs) = xmon;
int (*debugger_bpt)(struct pt_regs *regs) = xmon_bpt;
int (*debugger_sstep)(struct pt_regs *regs) = xmon_sstep;
int (*debugger_iabr_match)(struct pt_regs *regs) = xmon_iabr_match;
int (*debugger_dabr_match)(struct pt_regs *regs) = xmon_dabr_match;
void (*debugger_fault_handler)(struct pt_regs *regs);
#else
#ifdef CONFIG_KGDB
void (*debugger)(struct pt_regs *regs);
int (*debugger_bpt)(struct pt_regs *regs);
int (*debugger_sstep)(struct pt_regs *regs);
int (*debugger_iabr_match)(struct pt_regs *regs);
int (*debugger_dabr_match)(struct pt_regs *regs);
void (*debugger_fault_handler)(struct pt_regs *regs);
#endif
#endif

void reschedule(void);

/*----------------------------------------------------------------*/
/* Machine Check Handling                                         */
/*----------------------------------------------------------------*/

void
MachineCheckException(i370_interrupt_state_t *saved_regs)
{
	printk ("machine check \n");
	show_regs (saved_regs);
	print_backtrace (saved_regs->irregs.r13);
	i370_halt();
}

/*----------------------------------------------------------------*/
/* Program Check Handling                                         */
/*----------------------------------------------------------------*/

static int pc_monitor(i370_interrupt_state_t *, unsigned long,
                      unsigned short);
static int pc_unsupported(i370_interrupt_state_t *, unsigned long,
                          unsigned short);
int do_page_fault(i370_interrupt_state_t *, unsigned long,
                  unsigned short);

static int pc_addressing(i370_interrupt_state_t *, unsigned long,
                          unsigned short);

static int pc_operation(i370_interrupt_state_t *, unsigned long,
                        unsigned short);

static int pc_math(i370_interrupt_state_t *, unsigned long,
                   unsigned short);

static pc_handler pc_table[] = {
	{-1,                   pc_unsupported}, /* 0 */
	{PIC_OPERATION,        pc_operation},
	{PIC_PRIVLEDGED,       pc_unsupported},
	{PIC_EXECUTE,          pc_unsupported},
	{PIC_PROTECTION,       do_page_fault},
	{PIC_ADDRESSING,       pc_addressing},
	{PIC_SPECIFICATION,    pc_operation},
	{PIC_DATA,             pc_unsupported},
	{PIC_FIXED_OVERFLOW,   pc_math}, /* 0x8 */
	{PIC_FIXED_DIVIDE,     pc_math},
	{PIC_DECIMAL_OVERFLOW, pc_math},
	{PIC_DECIMAL_DIVIDE,   pc_math},
	{PIC_EXP_OVERFLOW,     pc_math},
	{PIC_EXP_UNDERFLOW,    pc_math},
	{PIC_SIGNIFICANCE,     pc_math},
	{PIC_FP_DIVIDE,        pc_math},
	{PIC_SEGEMENT_TRANS,   do_page_fault},  /* 0x10 */
	{PIC_PAGE_TRANS,       do_page_fault},
	{PIC_TRANSLATION,      pc_unsupported},
	{PIC_SPECIAL_OP,       pc_unsupported},
	{PIC_PAGEX,            pc_unsupported},
	{PIC_OPERAND,          pc_operation},
	{PIC_TRACE_TABLE,      pc_unsupported}, /* Handled in head.S */
	{PIC_ASN_TRANS,        pc_unsupported},
	{-1,                   pc_unsupported}, /* 0x18 */
	{PIC_VECTOR_OP,        pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_SPACE_SWITCH,     pc_unsupported},
	{PIC_SQROOT,           pc_unsupported},
	{PIC_UNNORMALIZED,     pc_unsupported},
	{PIC_PC_TRANS,         pc_unsupported},
	{PIC_AFX_TRANS,        pc_unsupported}, /* 0x20 */
	{PIC_ASX_TRANS,        pc_unsupported},
	{PIC_LX_TRANS,         pc_unsupported},
	{PIC_EX_TRANS,         pc_unsupported},
	{PIC_PRIMARY_AUTH,     pc_unsupported},
	{PIC_SECONDARY_AUTH,   pc_unsupported},
	{PIC_LFX_TRANS,        pc_unsupported},
	{PIC_LSX_TRANS,        pc_unsupported},
	{PIC_ALET_SPEC,        pc_unsupported}, /* 0x28 */
	{PIC_ALEN_TRANS,       pc_unsupported},
	{PIC_ALE_SEQ,          pc_unsupported},
	{PIC_ASTE_VALIDITY,    pc_unsupported},
	{PIC_ASTE_SEQ,         pc_unsupported},
	{PIC_EXTENDED_AUTH,    pc_unsupported},
	{PIC_LSTE_SEQ,         pc_unsupported},
	{PIC_ASTE_INST,        pc_unsupported},
	{PIC_STACK_FULL,       pc_unsupported}, /* 0x30 */
	{PIC_STACK_EMPTY,      pc_unsupported},
	{PIC_STACK_SPEC,       pc_unsupported},
	{PIC_STACK_TYPE,       pc_unsupported},
	{PIC_STACKOP,          pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported}, /* 0x38 */
	{PIC_RGN_1_TRANS,      pc_unsupported},
	{PIC_RGN_2_TRANS,      pc_unsupported},
	{PIC_RGN_3_TRANS,      pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_ASCE_TYPE,        pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_MONITOR,          pc_monitor} /* 0x40 */
};

void
ProgramCheckException(i370_interrupt_state_t *saved_regs)
{
	unsigned long pfx_prg_code = *((unsigned long *) PFX_PRG_CODE);
	unsigned long pfx_prg_trans = *((unsigned long *) PFX_PRG_TRANS);

	unsigned short pic_code, per_event;

	pic_code  = pfx_prg_code & 0x00ff;
	per_event = pic_code & 0x0080;
	pic_code  &= 0x007f;
	if (per_event) {
		printk("PER event triggered\n");
	}
	pc_table[pic_code].pc_flih(saved_regs, pfx_prg_trans, pic_code);
}

static int
pc_monitor(i370_interrupt_state_t *saved_regs,
           unsigned long  trans,
           unsigned short code)
{
	printk ("Monitor Event Triggered\n");
	return(0);
}

/* Most of these are fatal, because we should not be getting them. */
static int
pc_unsupported(i370_interrupt_state_t *saved_regs,
               unsigned long  trans,
               unsigned short code)
{
	if (code == PIC_TRANSLATION)
		printk ("Error: Unexpected Translation-Specification Exception\n");
	else
		printk ("Program Check Unsupported code=0x%x trans=0x%lx\n",
		        code, trans);
	show_regs (saved_regs);
	print_backtrace (saved_regs->irregs.r13);
	i370_halt();
	return(1);
}

/* This is hit by translation exceptions. Usually some access of a
 * wild pointer by the kernel. e.g. printk(%s instead of %x. Heh.
 *
 * Unclear to me why wild pointers don't go through 0x10 or 0x11.
 * Currently, we don't expect faults in the kernel. So this is not
 * attached to do_page_fault().
 *
 * Principles of Operation gives an assortment of reasons:
 * The dispatchable-unit-control table,
 * the primary ASN-second-table entry,
 * entries in the access list, region first table,
 * region second table, region third table,
 * segment table,
 * page table,
 * linkage table,
 * entry table,
 * ASN first table, ASN second table,
 * authority table, linkage stack, and trace table.
 * Phew!
 */
static int pc_addressing(i370_interrupt_state_t *saved_regs,
                          unsigned long  trans,
                          unsigned short code)
{
	printk ("\nUnexpected Addressing Exception trans=0x%lx\n", trans);
	printk ("Probably due to a wild kernel pointer.\n");
	show_regs (saved_regs);
	print_backtrace (_get_SP());
	i370_halt();
	return(1);
}

/* Operation Exception thrown when instruction is not recognized or
 * if an operand is invalid. Specification Exception thrown if the PSW
 * is invalid, if a double-word operand uses an odd register, if a
 * float poing insn uses registers incorrectly.
 *
 * Fatal if in the kernel, should be converted to SIGBUS or something
 * for userland. (User tries to exeute something that isn't code.)
 */
static int pc_operation(i370_interrupt_state_t *saved_regs,
                        unsigned long  trans,
                        unsigned short code)
{
	if (user_mode(current->tss.regs)) {
		printk ("User-space Operation exception code=%x trans=0x%lx\n",
		         code, trans);
		printk("TODO: implement me by sending a signal to the user.\n");
		show_regs (saved_regs);
		print_backtrace (saved_regs->irregs.r13);
		i370_halt();
		return(1);
	}

	/* Fatal in the kernel! */
	if (code == 1)
		printk ("Fatal: Operation/Operand exception trans=0x%lx\n", trans);
	else if (code == 6)
		printk ("Fatal: Specification exception trans=0x%lx\n", trans);
	else
		printk ("Fatal: Unsupported exception code=%x\n", code);

	show_regs (saved_regs);
	print_backtrace (saved_regs->irregs.r13);
	i370_halt();
	return(1);
}

/* Math-related exception. Fatal if in kernel, convert to SIGFPE
 * for userland.
 */
static int pc_math(i370_interrupt_state_t *saved_regs,
                   unsigned long  trans,
                   unsigned short code)
{
	printk ("Math (floating point) exception trans=0x%lx\n", trans);
	printk("Not implemented\n");
	show_regs (saved_regs);
	print_backtrace (saved_regs->irregs.r13);
	i370_halt();
	return(1);
}

/*----------------------------------------------------------------*/
/* System Restart Handling                                        */
/*----------------------------------------------------------------*/
void
RestartException(i370_interrupt_state_t *saved_regs)
{
	printk ("System restart not supported\n");
	i370_halt();
}

/*----------------------------------------------------------------*/
/* Input and Output Interrupt Handling                            */
/*----------------------------------------------------------------*/

void do_IRQ(int, struct pt_regs *, unitblk_t *);

void
InputOutputException(i370_interrupt_state_t *saved_regs)
{
	schib_t schib;
	int     rc, irq;
	long pfx_io_parm = *((long *) PFX_IO_PARM);
	long pfx_subsys_id = *((long *) PFX_SUBSYS_ID);
	unitblk_t *ucb = (unitblk_t *) pfx_io_parm;

	do {
		rc = _tsch(pfx_subsys_id, &ucb->unitirb);
		/* The use of rc here can cause trouble if you cheat in your
		   drivers. A value of rc==0 is returned when the subchannel
		   was in status-pending, and the TSCH successfully cleared
		   that status.  However, if *someone else* (who?) already
		   cleared the pending status, then rc==1, and the IRQ handler
		   won't be called. Which may be a surprise: we got the execption,
		   that's why we are here, but "someone" already mucked with
		   the subchannel.  Oh well.  */
		if (!rc) {
			rc = _stsch(pfx_subsys_id, &schib);
			irq = schib.isc;
			do_IRQ(irq, saved_regs, ucb);
		}
		rc = _tpi(&pfx_io_parm);
	} while (rc != 0);

	/* Well, the do_IRQ probably gave someone something to do.
	   So go schedule whomever needs something to be done. */
	reschedule();
}

/*----------------------------------------------------------------*/
/* External Interrupt Handling                                    */
/*----------------------------------------------------------------*/
static void ei_time_slice(i370_interrupt_state_t *, unsigned short);
static void ei_iucv(i370_interrupt_state_t *, unsigned short);
static void ei_unsupported(i370_interrupt_state_t *, unsigned short);

static ei_handler ei_table[] = {
 {EI_INTERRUPT_KEY,  ei_unsupported},
 {EI_TOD_CLOCK_SYNC, ei_unsupported},
 {EI_CLOCK_COMP,     ei_time_slice},
 {EI_CPU_TIMER,      ei_unsupported},
 {EI_MALFUNCTION,    ei_unsupported},
 {EI_EMERGENCY,      ei_unsupported},
 {EI_CALL,           ei_unsupported},
 {EI_SERVICE,        ei_unsupported},
 {EI_IUCV,           ei_iucv},
 {0,                 ei_unsupported},
};

void
ExternalException (i370_interrupt_state_t *saved_regs)
{
	unsigned short code, i_ei;

	/* get the interruption code */
	code = *((unsigned long *) PFX_EXT_CODE);
	for (i_ei = 0; ei_table[i_ei].ei_code != 0; i_ei++) {
		if (code == ei_table[i_ei].ei_code) break;
	}
	ei_table[i_ei].ei_flih(saved_regs, code);
}

static void
ei_iucv(i370_interrupt_state_t *saved_regs,
        unsigned short code)
{
	printk ("IUCV external exception code=0x%x\n", code);
	return;
}

static void
ei_unsupported(i370_interrupt_state_t *saved_regs,
               unsigned short code)
{
	if (code == 0x40)
		printk ("\nInterrupt key pressed, halting\n");
	else
		printk ("Unexpected external exception, code=0x%x, halting\n", code);
	show_regs (saved_regs);
	print_backtrace (saved_regs->irregs.r13);
	i370_halt();
}

static void
ei_time_slice(i370_interrupt_state_t *saved_regs,
             unsigned short code)
{
	unsigned long long ticko;
	/* get and set the new clock value */
	/* clock ticks every 250 picoseconds (actually, every
	 * microsecond/4K). We want an interrupt every HZ of
	 * a second */

#if defined (CONFIG_VM_GUEST) || defined (CONFIG_HERCULES_GUEST)
	/* It is more correct to use stckc as this avoids clock skew.
	 * However, on VM, if VM is overburdened, then we risk never
	 * catching up.  So use stck for VM ...
	 * We're also going to make the clock tick 20 times a second,
	 * not 100, since that makes debugging easier.
	 * Gee, I wonder if we should #define HZ to 20 not 100???
	 */
	ticko = _stck ();
	ticko += (5000000/HZ) << 12;   // effective HZ = 20 per second
	_sckc (ticko);
#else
	ticko = _stckc ();
	ticko += (1000000/HZ) << 12;  // Effective HZ = 100 per second
	_sckc (ticko);
#endif /* CONFIG_VM_GUEST */

	/* let Linux do its timer thing */
	do_timer (saved_regs);

	/* timer interrupts are a great time to reschedule */
	reschedule();
}

/* ================================================================ */
/* Do SLIH interrupt handling (the bottom half) */
int check_stack(struct task_struct *tsk);
int do_signal(sigset_t *oldset, struct pt_regs *regs);

void
reschedule (void)
{
	int do_it_again = 1;

	/* If we are already in the bottom half on this stack,
	 * then return. We don't want to blow out the stack
	 * by piling a million interrupts on it.
	 */
	if (current->tss.in_slih) return;
	current->tss.in_slih = 1;
#define CHECK_STACK
#ifdef CHECK_STACK
	check_stack(current);
#endif
	/* When we enable interrupts with sti(), we'll get a shower
	 * of interrupts here. The in_slih flag will keep them
	 * returning to here quickly enough.  Handle them, and
	 * then keep looping until they're all gone.
	 */
	while (do_it_again) {
		sti();    /* enable interrupts */
		do_it_again = 0;
#ifdef CHECK_STACK
		check_stack(current);
#endif

		/* bitwise and */
		if (bh_mask & bh_active) {
			do_bottom_half ();  // handle_bottom_half()
			do_it_again = 1;
		}

		/* Are we returning to the user? */
		if (user_mode(current->tss.regs)) {

			/* Reschedule before delivering signals */
			if (current->need_resched) {
				schedule ();
				do_it_again = 1;
				continue;
			}

			/* If we are here, we were just scheduled.
			 * So deliver any pending signals before returning. */
			if (current->sigpending) {
				i370_do_signal (NULL, current->tss.regs);
				do_it_again = 0;
				continue;
			}
		} else {
			if (current->need_resched) {
				schedule ();
				do_it_again = 1;
				continue;
			}
		}
	}

	/* Return to FLIH with interrupts disabled. */
	cli ();
	current->tss.in_slih = 0;
}

/* ================================================================ */


void
StackOverflow(struct pt_regs *regs)
{
	printk(KERN_CRIT "Kernel stack overflow in process %p, sp=%lx\n",
	        current, regs->irregs.r13);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	debugger(regs);
#endif
	show_regs(regs);
	print_backtrace (regs->irregs.r13);
	i370_halt();
}

/* ================================================================ */
/* wire in the interrupt vectors */

extern void SupervisorCall (void);
extern void External (void);
extern void InputOutput (void);
extern void ProgramCheck (void);

/*
 * We initialize interrupt vectors twice:
 * first with storage key zero, so that we can take timer interrupts while
 * booting, and then, with a non-zero storage key so that we can run
 * the interrupts without clobbering text pages.
 */

__initfunc(void i370_trap_init (int key))
{
	unsigned long long clock_reset;
	unsigned long *sz;
	psw_t psw;

	key >>= 4;
	key &= 0xf;
	// printk ("Trap init with storage key=%d\n", key);

	/* Do part of the initialization just once. */
	if (0 == key) {
		/* Clear any pending timer interrupts before installing
		 * external interrupt handler. Do this by setting the
		 * clock comparator to -1.
		 */
		clock_reset = 0xffffffffffffffffLL;
		_sckc (clock_reset);

		/* Store the offset between the task struct and the kernel stack
		 * pointer in low memory where we can get at it for calculation.
		 */
		sz = (unsigned long *) INTERRUPT_BASE;
		*(sz-1) = (unsigned long) &(((struct task_struct *) 0) ->tss.ksp);
		*(sz-2) = (unsigned long) &(((struct task_struct *) 0) ->tss.regs);
	}

	/* All interrupts will (eventually) run in execution key 6 ... */

	// Install the SVC handler.
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interrupts
	psw.addr = ((unsigned long) SupervisorCall) | PSW_31BIT;
	*((psw_t *) SVC_PSW_NEW) = psw;

	// Install the External Interrupt (clock) handler.
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interrupts
	psw.addr = ((unsigned long) External) | PSW_31BIT;
	*((psw_t *) EXTERN_PSW_NEW) = psw;

	// Install the I/O Interrupt handler.
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interrupts
	psw.addr = ((unsigned long) InputOutput) | PSW_31BIT;
	*((psw_t *) IO_PSW_NEW) = psw;

	// restart -- quick hack
	psw.flags = 0x000a0000;
	psw.addr = 0x0000fffc;
	*((psw_t *) IPL_PSW_NEW) = psw;

	// Install the ProgramCheck handler.
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interrupts
	psw.addr = ((unsigned long) ProgramCheck) | PSW_31BIT;
	*((psw_t *) PROG_PSW_NEW) = psw;
}

/*
 * Init traps with the keys to the kingdom.
 */

__initfunc(void trap_init(void))
{
	i370_trap_init (0);
}

/* ===================== END OF FILE =================================== */
