
/* XXX totally bogus */

.using	r0
	
	.globl	_start
_start:

/*
 * Exception vectors.
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

/* Machine Check */
	. = 0x200
MachineCheck:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler

/* Supervisor Call */
	. = 0x300
SupervisorCall:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler

/* Program */
	. = 0x400
Program:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler

/* External */
	. = 0x500
External:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler

/* Input Output */
	. = 0x600
InputOutput:
	EXCEPTION_PROLOG 
	bas 	r0, transfer_to_handler


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
