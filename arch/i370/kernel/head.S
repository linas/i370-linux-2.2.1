/*
 * implementation of startup code for ESA/390
 */

#include <asm/processor.h>

/* start of text section */
        .text
        .globl  _stext
_stext:

/* first instruction executed is here */
/* 
 * We set up just enough to let a C lang call to be made.
 * Basically, this means just reserving some space for 
 * a stack and making r13 point at it. That's all, and we can call.
 *
 * Note that we will be using this stack not just for startup,
 * but also for the exception handlers below ...
 * 
 * Note that this stack is kind of bogus, and should be replaced by
 * something that has a guard page at the end of it.
 *
 * Register naming convention:
 * lr == r14 == link reg
 * sp == r13 == stack pointer
 * rtca == r12 
 */
	.section .data.init	// we throw this section away after init
	.balign	8

        .text			// back to the text section
	.globl	_start
_start:
	BASR	r15,0		// load r15 with PSW 
	.using	.,r15		// use r15 as base register
	L	sp,=A(init_ksp)	// load address of stack pointer into r13
	L	sp,0(,sp)	// load address of stack into r13
	LA	r2,148(,sp)	// load r2 with top of stack
	ST	r2,76(,sp)	// store top of stack at 76
	L	rtca,=A(init_tca)	// load address of pointer to tca.
	L	rtca,0(,rtca)		// load address of tca
	L	r1,=A(init_kstend)	// r1 == address of pointer end of stack
	L	r1,0(,r1)		// r1 == end of stack
	ST	r1,12(,rtca)		// store end of stack in the tca
	L	r1,=A(tcaStackOverflow)	// bogus bogus hack alert XXX
	ST	r1,116(,rtca)		// bogosity
	L	r15,=A(start_kernel)	// load address of C routine
	BASR	lr,r15		// jump to C routine
	.balign	4		// align literal pool
	.ltorg			// dump literal pool


tcaStackOverflow:
	SVC	0xff		// force a fault to occur

/* ================================================================ */
/*
 * Exception vectors.
 * XXX currently sorta bogus code
 */

/*
 * Exception handling code.  
 * Does just enough work to call a C routine.
 * Takes as arguments the exception handler to call, and 
 * a temporary scratch area in the prefix page.
 * -- Save just some of the registers; the C prologue will
 *    save the rest on stack.
 * -- The C routine will return the double-word PSW on the stack 
 *
 * We check the psw to see if we were in problem state or supervisor 
 * state. If we were interrupted from supervisor state, then the 
 * kernel stack pointer and tca are already valid and set up, so we 
 * just go.  If we were interrupted from problem state, then r13
 * contains the user's stack pointer; we need to track down the kernel 
 * stack pointer and set that up before launching into C.
 *
 * Memory map:
 * starting at sav, the memory map should match the memory layout 
 * of the C struct _i370_regs_s exactly; a pointer to this struct is
 * taken as input to the called C function, and this memory is copied
 * to on return.
 * 
 * Floating point regs get saved only during task switch, and not here ...
 *
 * sav-8   offset from struct task_struct to tca
 * sav-4   offset from struct task_struct to ksp
 * sav     psw flags
 * sav+4   psw addr
 * sav+8   saved r11
 * sav+12  saved r12
 * sav+16  saved r13
 * sav+20  saved r14
 * sav+24  saved r15
 * sav+28  saved r0
 * sav+32  saved r1
 * sav+36  saved r2
 */
#define EXCEPTION_HANDLER(sav,oldpsw,handler)  				\
	STM	r11,r2,sav+8(0);	/* save gprs to low mem */	\
	BASR	r15,0;			/* load r15 with PSW  */	\
	.using	.,r15;			/* use r15 as base register */	\
	L	r1,oldpsw(,0);		/* get old psw flags into r1 */	\
	N	r1,=X'00010000';	/* is in problem state? */	\
	BZ	1f;			/* sp is already ok if kern */	\
	L	r11,=V(current);	/* addr of current pointer */	\
	L	r11,0(,r11);		/* addr of current process */	\
	LA	sp,0(,r11);		/* copy r11 to r13 */		\
	A	sp,sav-4(,0);		/* add offset to ksp */		\
	L	sp,0(,sp);		/* load kernel sp */		\
	LA	rtca,0(,r11);		/* copy r12 to r13 */		\
	A	rtca,sav-8(,0);		/* add offset to tca */		\
	/* XXX the following 6 insns can be chopped out later */	\
	LA	r15,148(,sp);		/* load 15 with top of stack */	\
	ST	r15,76(,sp);		/* store top of stack at 76 */	\
	LA	r1,4092(,r11);		/* r1 == end of stack */	\
	ST	r1,12(,rtca);		/* store end of stack in tca */ \
	L	r1,=A(tcaStackOverflow);/* bogus hack alert XXX */	\
	ST	r1,116(,rtca);		/* bogosity */			\
1:	L	r15,=A(handler);	/* load address of C routine */	\
	LA	r11,sav(,0);		/* pointer to arguments */	\
	LA	r1,sav(,0);		/* pointer to returned value */	\
	BASR	lr,r15;			/* jump to C routine */		\
	LM	r11,r2,sav+8(0);	/* restore saved gprs */	\
	LPSW	sav(0);			/* return from interrupt */	\
	.balign	8;			/* align literal pool */	\
	.ltorg;				/* dump literal pool */		\

/* Restart */
	.globl Restart
Restart:
	EXCEPTION_HANDLER (INTERRUPT_BASE, IPL_PSW_OLD, RestartException)

/* Machine Check */
	.globl MachineCheck
MachineCheck:
	EXCEPTION_HANDLER (INTERRUPT_BASE, MACH_PSW_OLD, MachineCheckException)

/* Supervisor Call */
	.global SupervisorCall
SupervisorCall:
	EXCEPTION_HANDLER (INTERRUPT_BASE, SVC_PSW_OLD, SupervisorCallException) 

/* Program Interruption */
	.global ProgramCheck
ProgramCheck:
	EXCEPTION_HANDLER (INTERRUPT_BASE, PROG_PSW_OLD, ProgramCheckException)

/* External */
	.global External
External:
	EXCEPTION_HANDLER (INTERRUPT_BASE, EXTERN_PSW_OLD, ExternalException)

/* Input Output */
	.global InputOutput
InputOutput:
	EXCEPTION_HANDLER (INTERRUPT_BASE, IO_PSW_OLD, InputOutputException)


/*
 * XXX ??? XXX not aligned any more, huh ??
 * We put a few things here that have to be page-aligned.
 * This stuff goes at the beginning of the data segment,
 * which is page-aligned.
 */
	.data
	.globl	sdata
sdata:
	.globl	empty_zero_page
empty_zero_page:
	.space	4096

	.globl	swapper_pg_dir
swapper_pg_dir:
	.space	4096	

/*
 * This space gets a copy of optional info passed to us by the bootstrap
 * Used to pass parameters into the kernel like root=/dev/sda1, etc.
 */	
	.globl	cmd_line
cmd_line:
	.space	512
