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
 * a bogus stack (to which will be written garbage data)
 * and making r13 point at it. That's all, and we can call.
 */
	.data
	.balign	8
.LSTACK:
	.org	.+148, 0xd0	// all stack, no frame
	.balign	8

        .text			// back to the text section
	.globl	_start
_start:
	.using	.,r15		// use r15 as base register
	BASR	r15,0		// load r15 with PSW 
	B	.LSTART-_start	// jump over next word
.LLEN:
	.long	.LSTACK		// address of stack
.LSTART:
	L	sp,.LLEN-_start(0,r15)	// load address of stack into r13
	L	r15,=A(start_kernel)	// load address of C routine
	BASR	lr,r15		// jump to C routine

	.ltorg			// dump literal pool

/* ================================================================ */
/*
 * Exception vectors.
 * XXX currently totally bogus code
 */


/*
 * Exception entry code.  
 * save contents of registers. 
 */
#define EXCEPTION_PROLOG        \
        STM     r14,r12,12(sp)  /* totally wrong; placeholder */

/* Restart */
	. = 0x100
Restart:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler
	.long	RestartException

/* Machine Check */
	. = 0x200
MachineCheck:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler
	.long	MachineCheckException

/* Supervisor Call */
	. = 0x300
SupervisorCall:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler
	.long	SupervisorCallException

/* Program Interruption */
	. = 0x400
ProgramCheck:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler
	.long	ProgramCheckException

/* External */
	. = 0x500
External:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler
	.long	ExternalException

/* Input Output */
	. = 0x600
InputOutput:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler
	.long	InputOutputException


/*
 * This code finishes saving the registers to the exception frame
 * and jumps to the appropriate handler for the exception, turning
 * on address translation.
 */

	. = 0x1000
        .globl  transfer_to_handler
transfer_to_handler:



/*
 * This routine is just here to keep GCC happy - sigh...
 */	
	.global __main
__main:
	basr 1,lr

	
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
