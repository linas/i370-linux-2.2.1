/*
 * implementation of startup code for ESA/390
 */

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
	.data			// should probably be .section .data.init ?
	.balign	8
.LSTACK:
	.org	4096,0xd0	// "callers" stack, load with eyecatcher  
.LSTKEND:
	.balign	8

        .text			// back to the text section
	.globl	_start
_start:
	BASR	r15,0		// load r15 with PSW 
	.using	.,r15		// use r15 as base register
	L	sp,=A(.LSTACK)	// load address of stack into r13
	LA	r2,148(,sp)	// load r2 with top of stack
	ST	r2,76(,sp)	// store top of stack at 76
	LA	rtca,0(,sp)	// let bottom of stack double as tca
	L	r1,=A(.LSTKEND)	// r1 == end of stack
	ST	r1,12(,rtca)	// store end of stack in the tca
	L	r1,=A(StackOverflow)	// bogus bogus hack alert XXX
	ST	r1,116(,rtca)		// bogosity
	L	r15,=A(start_kernel)	// load address of C routine
	BASR	lr,r15		// jump to C routine
	.balign	4		// align literal pool
	.ltorg			// dump literal pool


/* ================================================================ */
/*
 * Exception vectors.
 * XXX currently totally bogus code
 */


/*
 * Exception handling code.  
 * Save contents of registers, call C routine.
 * The C routine will return the double-word PSW on the stack 
 */
#define EXCEPTION_HANDLER(handler)     				\
	STM	r0,r15,4008(0);	/* save gprs to low mem */	\
	.using	.,r15;		/* use r15 as base register */	\
	BASR	r15,0;		/* load r15 with PSW  */	\
	L	sp,=A(.LSTACK);	/* load addr of stack */	\
	L	r15,=A(handler);/* load address of C routine */	\
	LA	r1,4000(0,0);	/* pointer to arguments */	\
	BASR	lr,r15;		/* jump to C routine */		\
	LM	r6,r7,0(r15);	/* load double-word psw */	\
	STM	r6,r7,4000(0);	/* store returned psw */	\
	LM	r0,r15,4008(0);	/* restore saved gprs */	\
	LPSW	4000(0);	/* return from interrupt */	\
	.balign	8;		/* align literal pool */	\
	.ltorg;			/* dump literal pool */		\

/* Restart */
	.globl Restart
Restart:
	EXCEPTION_HANDLER(RestartException)

/* Machine Check */
MachineCheck:
	EXCEPTION_HANDLER(MachineCheckException)

/* Supervisor Call */
SupervisorCall:
	EXCEPTION_HANDLER(SupervisorCallException) 

/* Program Interruption */
ProgramCheck:
	EXCEPTION_HANDLER(ProgramCheckException)

/* External */
External:
	EXCEPTION_HANDLER(ExternalException)

/* Input Output */
InputOutput:
	EXCEPTION_HANDLER(InputOutputException)


/*
 * This routine is just here to keep GCC happy - sigh...
 */	
	.global __main
__main:
	basr r15,lr

	
/*
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
