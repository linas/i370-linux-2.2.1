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


extern void sys_call_table(void);
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
#ifdef LATER
void ret_from_syscall(void);
#endif

void instruction_dump (unsigned short *pc)
{
   int i;

   printk("Instruction DUMP:");
   for(i = -12; i < 6; i++)
      printk("%c%04x%c", i?' ':'<', pc[i], i?' ':'>' );
   printk("\n");
}


/*
 * Trap & Exception support
 */

void
_exception(int signr, struct pt_regs *regs)
{
	if (!user_mode(regs))
	{
		show_regs(regs);
		print_backtrace (regs->irregs.r13);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		debugger(regs);
#endif
		instruction_dump((unsigned short *)instruction_pointer(regs));
		printk ("Exception in kernel \n");
		i370_halt();
	}
	force_sig(signr, current);
}

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
static int pc_unsupported(i370_interrupt_state_t *,  unsigned long,
                       unsigned short);
int do_page_fault(i370_interrupt_state_t *,  unsigned long,
                  unsigned short);
 
static pc_handler pc_table[] = {
	{-1,                   pc_unsupported},
	{PIC_OPERATION,        pc_unsupported},
	{PIC_PRIVLEDGED,       pc_unsupported},
	{PIC_EXECUTE,          pc_unsupported},
        {PIC_PROTECTION,       do_page_fault},
	{PIC_ADDRESSING,       pc_unsupported},
	{PIC_SPECIFICATION,    pc_unsupported},
	{PIC_DATA,             pc_unsupported},
	{PIC_FIXED_OVERFLOW,   pc_unsupported},
	{PIC_FIXED_DIVIDE,     pc_unsupported},
	{PIC_DECIMAL_OVERFLOW, pc_unsupported},
	{PIC_DECIMAL_DIVIDE,   pc_unsupported},
	{PIC_EXP_OVERFLOW,     pc_unsupported},
	{PIC_EXP_UNDERFLOW,    pc_unsupported},
	{PIC_SIGNIFICANCE,     pc_unsupported},
	{PIC_FP_DIVIDE,        pc_unsupported},
        {PIC_SEGEMENT_TRANS,   do_page_fault},
        {PIC_PAGE_TRANS,       do_page_fault},
	{PIC_TRANSLATION,      pc_unsupported},
	{PIC_SPECIAL_OP,       pc_unsupported},
	{PIC_PAGEX,            pc_unsupported},
	{PIC_OPERAND,          pc_unsupported},
        {PIC_TRACE_TABLE,      pc_unsupported}, /* Handled in head.S */
	{PIC_ASN_TRANS,        pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_VECTOR_OP,        pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_SPACE_SWITCH,     pc_unsupported},
	{PIC_SQROOT,           pc_unsupported},
	{PIC_UNNORMALIZED,     pc_unsupported},
	{PIC_PC_TRANS,         pc_unsupported},
	{PIC_AFX_TRANS,        pc_unsupported},
	{PIC_ASX_TRANS,        pc_unsupported},
	{PIC_LX_TRANS,         pc_unsupported},
	{PIC_EX_TRANS,         pc_unsupported},
	{PIC_PRIMARY_AUTH,     pc_unsupported},
	{PIC_SECONDARY_AUTH,   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_ALET_SPEC,        pc_unsupported},
	{PIC_ALEN_TRANS,       pc_unsupported},
	{PIC_ALE_SEQ,          pc_unsupported},
	{PIC_ASTE_VALIDITY,    pc_unsupported},
	{PIC_ASTE_SEQ,         pc_unsupported},
	{PIC_EXTENDED_AUTH,    pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_STACK_FULL,       pc_unsupported},
	{PIC_STACK_EMPTY,      pc_unsupported},
	{PIC_STACK_SPEC,       pc_unsupported},
	{PIC_STACK_TYPE,       pc_unsupported},
	{PIC_STACKOP,          pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{-1,                   pc_unsupported},
	{PIC_MONITOR,          pc_monitor}
};
 

void
ProgramCheckException(i370_interrupt_state_t *saved_regs)
{
	unsigned long pfx_prg_code = *((unsigned long *) PFX_PRG_CODE);
	unsigned long pfx_prg_trans = *((unsigned long *) PFX_PRG_TRANS);

	unsigned short pic_code,
                per_event;
 
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
 
static int
pc_unsupported(i370_interrupt_state_t *saved_regs,
           unsigned long  trans,
           unsigned short code)
{
	printk ("Program Check Unsupported code=0x%x trans=0x%lx\n",
		code, trans);
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
		if (!rc) {
			rc = _stsch(pfx_subsys_id, &schib);
			irq = schib.isc;
			do_IRQ(irq, saved_regs, ucb);
		}
		rc = _tpi(&pfx_io_parm);
	} while (rc != 0);
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
	unsigned short code,
                i_ei;

	/*----------------------------------------------------------*/
	/* get the interruption code */
	/*----------------------------------------------------------*/
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
	printk ("unexpected external exception code=0x%x\n", code);
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

	/* It is more correct to use stckc as this avoids clock skew.
	 * However, on VM, if VM is very very overburdened, then we
	 * risk never catching up.  So use stck for VM ... */
#ifdef CONFIG_VM
	ticko = _stck ();
#else
	ticko = _stckc (); 
#endif
	ticko += (1000000/HZ) << 12;
	_sckc (ticko);
	
	/* let Linux do its timer thing */
	do_timer (saved_regs);
}

/* ================================================================ */
/* do SLIH intrerrupt handling (the bottom half) */
/* pseudocode:  */

#ifdef LATER

void 
ret_from_syscall (void) 
{
	int do_it_again = 1;

	/* if we are already in the bottom half on this stack,
	 * then return. We don't want to blow out the stack
	 * by piling a million interrupts on it.
	 */
	if (current->tss.in_slih) return;
	current->tss.in_slih = 1;

	/* keep looping until there are no more pending 
	 * interrupts. */
	while (do_it_again) {
		sti();
		do_it_again = 0:

		/* bitwise and */
		if (bh_mask & bh_active) {
			do_bottom_half ();  // handle_bottom_half()
			do_it_again = 1;
		}
		if (return_to_user) {
			/* reschedule before delivering signals */
			if (need_reschedule) {
				schedule ();
				do_it_again = 1;
				continue;
			}
			/* if we are here, we weren't scheduled away. 
                         * So deliver any pending signals before returning. */
			if (sig_pending) {
				do_signal ();
				do_it_again = 1;
				continue;
			}
		}
	}
	cli ();
	current->tss.in_slih = 0;
}
#endif

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
	// instruction_dump((unsigned long *)regs->psw.addr);
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
	printk ("trap init with storage key=%d\n", key);

	/* do part of the initialization just once. */
	if (0 == key) {
		/* clear any pending timer interrupts before installing
		 * external interrupt handler. Do this by setting the
        	 * clock comparator to -1.
		 */
		clock_reset = 0xffffffffffffffffLL;
		_sckc (clock_reset);
	 
		/* store the offset between the task struct and the kernel stack
		 * pointer in low memory where we can get at it for calculation
		 */
		sz = (unsigned long *) INTERRUPT_BASE;
		*(sz-1) = (unsigned long) &(((struct task_struct *) 0) ->tss.ksp);
		*(sz-2) = (unsigned long) &(((struct task_struct *) 0) ->tss.regs);
	}

	/* note that all interrupts will run in execution key 6 ... */
	// install the SVC handler
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interupts
	psw.addr = ((unsigned long) SupervisorCall) | PSW_31BIT; 
	*((psw_t *) SVC_PSW_NEW) = psw;

	// install the External Interrupt (clock) handler
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interupts
	psw.addr = ((unsigned long) External) | PSW_31BIT; 
	*((psw_t *) EXTERN_PSW_NEW) = psw;

	// install the I/O Interrupt handler
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interupts
	psw.addr = ((unsigned long) InputOutput) | PSW_31BIT; 
	*((psw_t *) IO_PSW_NEW) = psw;

	// restart quick hack
	psw.flags = 0x000a0000;
	psw.addr = 0x0000ffff;
	*((psw_t *) IPL_PSW_NEW) = psw;

	// install the ProgramCheck handler
	psw.flags = PSW_VALID | PSW_KEY(key);      // disable all interupts
	psw.addr = ((unsigned long) ProgramCheck) | PSW_31BIT; 
	*((psw_t *) PROG_PSW_NEW) = psw;
}

/*
 * init traps with the keys to the kingdom.
 */

__initfunc(void trap_init(void))
{
	i370_trap_init (0);
}

/* ===================== END OF FILE =================================== */
