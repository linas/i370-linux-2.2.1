/*
 * Implementation of startup code and FLIH entry/exit for ESA/390
 * Copyright (c) 1999 Linas Vepstas (linas@linas.org)
 */

#include <asm/errno.h>
#include <asm/processor.h>
#include <asm/psa.h>

/* Stop the machine. Handy for debugging purposes, but also
 * used in a few "it can't possibly happen" situations.
 */
#define HALT(sav)							\
	SPKA    sav(0);			/* Ensure we can use lo core */ \
	L	r1,=X'000A0000';	/* disabled wait             */	\
	ST	r1,sav(,0);						\
	L	r1,=A(1f);		/* must be any valid address */	\
	O	r1,=X'80000000'; 	/* must be any valid address */	\
	ST	r1,sav+4(,0);						\
	LPSW	sav(0);			/* oops halt the machine     */ \
1:	NOPR	r0;

/* First instruction executed is here */
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
 * r13 == frame & arg pointer (bottom of stack)
 * r11 == stack pointer (top of stack)
 */

/* start of text section */
        .text
        .globl  _stext
_stext:
	.globl	_start
_start:
	STM	r0,r15,0x300(0)		/* command line passed in regs    */

	BASR	r15,0			/* load r15 with PSW              */
	.using	.,r15			/* use r15 as base register       */

	/* make sure AMODE is 31, interrupts disabled                     */
	L	r1,=X'00080000'		/* everything disabled            */
	ST	r1,INTERRUPT_BASE(,0)	/* store into prefix page         */
	L	r1,=A(_cinit)		/* jump to cinit next             */
	O	r1,=X'80000000'		/* AMODE 31                       */
	ST	r1,INTERRUPT_BASE+4(,0)	/* store into prefix page         */
	LPSW	INTERRUPT_BASE(0)	/* jump to label                  */

	/* perform initialization to call C routines                      */
_cinit:
	L	r13,=A(init_ksp)	/* load addr of stackptr into r13 */
	L	r13,0(,r13)		/* load address of stack into r13 */
	LA	r11,MIN_FRAME_SIZE(,r13); /* load r11 with top of stack   */
	SLR	r3,r3
	ST	r3,8(,r13)		/* terminate stack backchain      */
	ST	r3,8(,r11)		/* terminate stack backchain      */
	ST	r3,64(,r13)		/* terminate stack backchain      */
	ST	r3,64(,r11)		/* terminate stack backchain      */
	L	r15,=A(start_kernel)	/* load address of C routine      */
	BASR	lr,r15			/* jump to C routine              */
	.drop	r15
	.using	.,lr			/* re-establish addressing        */
	HALT (INTERRUPT_BASE)		/* it's impossible to return here */

	.balign	4			/* align literal pool             */
	.ltorg				/* dump literal pool              */



/* ================================================================ */
/*
 * Exception vectors.
 */

/*
 * Exception handling code.
 * Does just enough work to call a C routine.
 * Macro takes as arguments the exception handler to call, and
 * address of a temporary scratch area in the prefix page.
 * -- Save just some of the registers; the C prologue will
 *    save the rest on stack.
 * -- The C routine will return the double-word PSW on
 *    in the prefix page
 *
 * We check the psw to see if we were in problem state or supervisor
 * state. If we were interrupted from supervisor state, then the
 * kernel stack pointer is OK and ready to go.
 *
 * If we were interrupted from problem state, then r13
 * contains the user's stack pointer; we need to track down the kernel
 * stack pointer and set that up before launching into C.
 *
 * Memory map:
 * starting at sav, the memory map should match the memory layout
 * of the C struct _i370_interrupts_state_s exactly; this struct is
 * saved on the stack and passed to C routines as a pointer.
 *
 * NB: Floating point regs get saved only during task switch, and not here ...
 *
 * sav-16  old interrupt mask ????
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

/* -------------------------------------------------------- */
/* Stack related offsets and total size.                    */
/* Double-word align.                                       */
/* -------------------------------------------------------- */
#define STACK_SIZE  0x58        /* sizeof (_i370_elf_stack_t) */
#define ARGS_BASE   STACK_SIZE
#define ARGP        ARGS_BASE   /* Callee's args in callee's frame */

/* Adjustments for catching exception while in kernel */
#define ARGS_SIZE   0x28        /* Enough for 10 args of 4 word each. */
#define FRAME_BASE  STACK_SIZE + ARGS_SIZE
#define FRB_SIZ     128         /* STACK_SIZE + ARGS_SIZE by hand */

#define EXF_SIZ     IR_SIZE     /* Exception frame size */

/* -------------------------------------------------------- */
/* Structure of our "exception frame"                       */
/* Location of saved registers on stack                     */
/* -------------------------------------------------------- */
#define SAVE_BASE 0
#define SAVED_PSW	SAVE_BASE + IR_PSW
#define SAVED_R11	SAVE_BASE + IR_R11
#define SAVED_R12	SAVE_BASE + IR_R12
#define SAVED_R13	SAVE_BASE + IR_R13
#define SAVED_R14	SAVE_BASE + IR_R14
#define SAVED_R15	SAVE_BASE + IR_R15
#define SAVED_R0	SAVE_BASE + IR_R0
#define SAVED_R1	SAVE_BASE + IR_R1
#define SAVED_R2	SAVE_BASE + IR_R2

#define SAVED_R3	SAVE_BASE + IR_R3
#define SAVED_R4	SAVE_BASE + IR_R4
#define SAVED_R5	SAVE_BASE + IR_R5
#define SAVED_R6	SAVE_BASE + IR_R6
#define SAVED_R7	SAVE_BASE + IR_R7
#define SAVED_R8	SAVE_BASE + IR_R8
#define SAVED_R9	SAVE_BASE + IR_R9
#define SAVED_R10	SAVE_BASE + IR_R10

#define SAVED_OLDREGS	SAVE_BASE + IR_OLDREGS

/* We don't have to save all; only those clobbered by
 * EXCEPTION_PROLOG and friends. Currently, we clobber
 * r11 r12 r13 r14 r15 r1 r2 r3
 *
 * If we don't STM r5,r10 here, then do it below, near the MVC.
 * Like this, except this blows up. Why ???
 *	MVC	SAVED_R11(40,r13),sav+IR_R11(0);  / * 10 regs=40 bytes * /
 *	STM	r5,r10,SAVED_R5(r13);	/ * those we didn't touch * /
 */
#define SAVE_REGS(sav) 							\
	STM	r11,r4,sav+IR_R11(0);	/* save gprs to PSA scratch */	\
	STM	r5,r10,sav+IR_R5(0);    /* for debugging only, remove later xxx */


#define EXCEPTION_PROLOG(sav,oldpsw) 					\
	BASR	r15,0;			/* load r15 with PSW         */	\
	.using	.,r15;			/* use r15 as base register  */	\
	L	r12,_psa_current(,0);	/* addr of current process   */	\
	/* Interrupted from user space or kernel space?              */	\
	L	r1,oldpsw(,0);		/* get old psw flags into r1 */	\
	N	r1,=X'00010000';	/* is in problem state?      */	\
	BZ	1f;			/* sp is already ok if kern  */	\
	/* Interrupted user proc, so need to find a kernel stack.    */	\
	L	r13,OFFSET_KSP(,r12);   /* framep is stack bot       */	\
	B	2f;                     /*                           */	\
1:					/* branch here if kernel     */	\
	/* We have valid top-of-stack ptr (r11). Make room,          */	\
	/* just in case the exception hits in the function prolog.   */	\
	LA	r13,FRB_SIZ(r11);	/* Make room, for prolog.    */	\
2:					/*                           */	\
	LA	r11,EXF_SIZ(,r13);	/* Add stackframe size.      */	\
	SLR	r3,r3;			/* zero                      */	\
	ST	r3,IR_CALLER(,r13);	/* terminate stack backchain */	\
	/* move saved regs out of prefix page to stackframe          */	\
	MVC	SAVED_PSW(8,r13),oldpsw(0); /* save old psw on stack */	\
	MVC	SAVED_R11(64,r13),sav+IR_R11(0); /* 16 regs=64 bytes */	\
	/* chain the interrupt state pointer into thread_struct      */	\
	LA	r1,OFFSET_KREGS(,r12);	/* add offset to regs ptr    */ \
	L	r2,0(,r1);		/* load regs ptr             */	\
	ST	r2,SAVED_OLDREGS(,r13); /* save regs ptr to stack    */	\
	LA	r2,SAVE_BASE(,r13);	/* put addr of newregs in r2 */	\
	ST	r2,0(,r1);		/* update regs ptr in proc   */	\


#define EXCEPTION_CALL_C(handler)  					\
	L	r15,=A(handler);	/* load address of C routine */	\
	LA	r2,SAVE_BASE(,r13);	/* ptr to saved psw, regs    */	\
	ST	r2,ARGP(,r11);		/* ptr to arguments on stack */	\
	BASR	lr,r15;			/* jump to C routine         */	\
	.using	.,lr;			/* use r14 to the end        */	\


#define EXCEPTION_UNCHAIN(sav)	 					\
	/* Set struct task_struct * current->tss.regs = saved regs   */	\
	L	r12,_psa_current(,0);	/* struct task * current     */	\
	ST	r13,OFFSET_KSP(r12);	/* current->tss.ksp = r13    */ \
	MVC	OFFSET_KREGS(4,r12),SAVED_OLDREGS(r13);


#define EXCEPTION_RETURN(sav)  						\
	/* get old PSW, restore registers, return from interrupt     */	\
	MVC	sav(8,0),SAVED_PSW(r13);/* restore old psw frm stack */	\
	LM	r11,r12,SAVED_R11(r13);	/* restore r11, r12          */	\
	LM	r14,r4,SAVED_R14(r13);	/* restore r14 to r4         */	\
	LM	r5,r10,SAVED_R5(r13);   /* restore r5 thru r10       */	\
	L	r13,SAVED_R13(r13);	/* restore r13               */	\
	LPSW	sav(0);			/* return from interrupt     */


#define EXCEPTION_EPILOG(sav)  						\
	EXCEPTION_UNCHAIN(sav)						\
	EXCEPTION_RETURN(sav)

/* Areas used in the prefix page used for tracing */
#define	PFX_TRACE	0xd00

/* ID numbers not addresses */
#define IPL_TRC_ID   0x100
#define MCH_TRC_ID   0x200
#define SVC_TRC_ID   0x300
#define PRG_TRC_ID   0x400
#define EXT_TRC_ID   0x500
#define IO_TRC_ID    0x600

// #define DO_TRACING
#ifdef DO_TRACING
#define EXCEPTION_TRACE(trc_id,subid,irpt_code,old_psw)			\
	SLR	r0,r0;			/* Not Used */			\
	LA	r1,trc_id(0);		/* Trace id */			\
	ST	r1,PFX_TRACE(0);	/* Store in PFX */		\
	L	r1,subid(0);		/* subchannel id */		\
	L	r2,irpt_code(0);	/* Get Irpt code */		\
	LM	r3,r4,old_psw(0);	/* Get Old PSW */		\
	TRACE	r0,r4,PFX_TRACE(0);	/* Trace Wondrous event */
#else
#define EXCEPTION_TRACE(trc_id,subid,irpt_code,old_psw)
#endif

/* Restart IPL (Initial Program Load) */
	.globl Restart
Restart:
	SAVE_REGS (INTERRUPT_BASE)
	EXCEPTION_TRACE (IPL_TRC_ID, IPL_CCW1, IPLCCW2, IPL_PSW_OLD)
	EXCEPTION_PROLOG (INTERRUPT_BASE, IPL_PSW_OLD)
	EXCEPTION_CALL_C (RestartException)
	EXCEPTION_EPILOG (INTERRUPT_BASE)

/* Machine Check */
	.globl MachineCheck
MachineCheck:
	SAVE_REGS (INTERRUPT_BASE)
	EXCEPTION_TRACE (MCH_TRC_ID, PFX_MCH_CODE_LO, PFX_MCH_CODE_HI, MACH_PSW_OLD)
	EXCEPTION_PROLOG (INTERRUPT_BASE, MACH_PSW_OLD)
	EXCEPTION_CALL_C (MachineCheckException)
	EXCEPTION_EPILOG (INTERRUPT_BASE)

/* Supervisor Call */
#define NR_syscalls	189
	.global SupervisorCall
SupervisorCall:
	SAVE_REGS (INTERRUPT_BASE)
	EXCEPTION_TRACE (SVC_TRC_ID, 0, PFX_SVC_CODE, SVC_PSW_OLD)
	EXCEPTION_PROLOG (INTERRUPT_BASE, SVC_PSW_OLD)

	SLR	r2,r2			/* Put zero in r2             */
	LH	r2,138(,0)		/* Interruption code from pfx */
	LTR	r2,r2			/* Is r2 zero ?               */
	BZ	3f			/* Branch if zero             */
	HALT (INTERRUPT_BASE)		/* Stop the machine (debug)   */
3:
	L	r1,SAVED_R1(sp)		/* Get syscall number         */
	LA	r2,NR_syscalls(,0)	/* Max of NR_syscalls.        */
	CR	r1,r2			/* Is syscall less than NR?   */
	BL	4f			/* Branch if less.            */
	LA	r2,ENOSYSCALL(,0)	/* Load r2 with errno.        */
	ST	r2,SAVED_R15(,r13)	/* Return r15 to caller.      */
	ST	r1,SAVED_R1(,r13)	/* Return r1 to caller.       */
	EXCEPTION_EPILOG (INTERRUPT_BASE) /* Return from interrupt.   */
4:
	L	r2,=A(sys_call_table)	/* Address of sys call table. */
	SLL	r1,2			/* 4 bytes per word ...       */
	L	r15,0(r2,r1)		/* Load address of C routine. */
	STM	r5,r10,SAVED_R5(r13)	/* Sys call args in regs.     */
	STM	r5,r10,ARGP(r11)	/* Args on stack.             */
	LA	r1,ARGP(,r13)		/* Pointer to returned value. */
	BASR	lr,r15			/* Jump to C routine.         */
	.using	.,lr			/* Establish PC addressing    */
	STNSM	INTERRUPT_BASE-16(0),0xfc /* Disable interrupts.      */

	LR	r5,r15			/* Cache for later.           */
	L	r15,=A(ret_from_syscall)/* Reschedule; signals        */
	BASR	lr,r15			/* Do bottom halves.          */
	.using	.,lr			/* Establish PC addressing    */
	LTR	r15,r15                 /* Pending signals?           */
	BNZ	5f                      /* Yes. Return to sighandler. */
	LR	r15,r5			/* Restore r15 from syscall.  */
5:
	EXCEPTION_UNCHAIN (INTERRUPT_BASE) /* Unstack pointer chain.  */
	/* Get old PSW, restore registers, return from interrupt.     */
	/* r15 is not restored, but is returned 'as-is' from syscall. */
	MVC	INTERRUPT_BASE(8,0),SAVED_PSW(sp) /* Restore old psw. */
	LM	r0,r4,SAVED_R0(r13)	/* Restore r0 thru r4.        */
	/* If returning to userland, then restore r5 thru r10, also.  */
	/* We could do this conditionally, but not worth the effort.  */
	LM	r5,r10,SAVED_R5(r13)    /* Restore r5 thru r10.       */
	LM	r11,r12,SAVED_R11(r13)	/* Restore r11, r12.          */
	L	r14,SAVED_R14(r13)	/* Restore r14.               */
	L	r13,SAVED_R13(r13)	/* Restore r13.               */
	LPSW	INTERRUPT_BASE(0);	/* Return from interrupt.     */


#ifdef DO_TRACING
/* Trace Program Interruption */
#define TRACE_PC \
	BASR	r15,0 \
	.using	*,r15 \
	/*--------------------------------------------------------------*/ \
	/* If this is a trace table related program check then we'd     */ \
	/* better not generate a trace record or we could get ourselves */ \
	/* in a rather tight loop. In fact, we bypass nearly all proc-  */ \
	/* essing for the TRACE TABLE condition.                        */ \
	/*--------------------------------------------------------------*/ \
	L	r0,PFX_PRG_CODE(0) \
	LA	r14,0x7f(0) \
	NR	r0,r14 \
	LA	r14,PIC_TRACE_TABLE(0) \
	CR	r0,r14 \
	BNE	.DoPC \
 \
	STCTL	r12,r12,PRG_SAVE_CTX_C12(0) \
	L	r8,PRG_SAVE_CTX_C12(0) \
	N	r8,=X'7FFFF000' \
	L	r8,4088(,r8) \
	LA	r9,1(,0) \
	OR	r8,r9 \
	ST	r8,PRG_SAVE_CTX_C12(0) \
	LCTL	r12,r12,PRG_SAVE_CTX_C12(0) \
	LM	r11,r4,PC_INTERRUPT_BASE+IR_R11(0) \
	LM	r5,r10,PC_INTERRUPT_BASE+IR_R5(0) \
	LPSW	PROG_PSW_OLD(0) \
 \
.DoPC: \
	/*--------------------------------------------------------------*/ \
	/* We had better take a copy of the current context information */ \
	/* just in case the TRACE does cause a program check and we     */ \
	/* lose all our previous information (PSW, registers...).       */ \
	/*--------------------------------------------------------------*/ \
	MVC	PRG_SAVE_CTX_REGS(64,0),PC_INTERRUPT_BASE(0) \
	MVC	PRG_SAVE_CTX_PSW(8,0),PROG_PSW_OLD(0) \
	MVC	PRG_SAVE_CTX_CODE(4,0),PFX_PRG_CODE(0) \
	MVC	PRG_SAVE_CTX_TRANS(4,0),PFX_PRG_TRANS(0) \
	EXCEPTION_TRACE (PRG_TRC_ID, PFX_PRG_TRANS, PFX_PRG_CODE, PROG_PSW_OLD) \
	CLC	PRG_SAVE_CTX_PSW(8,0),PROG_PSW_OLD(0) \
	BE	.SkipRestore \
	MVC	PC_INTERRUPT_BASE(64,0),PRG_SAVE_CTX_REGS(0) \
	MVC	PROG_PSW_OLD(8,0),PRG_SAVE_CTX_PSW(0) \
	MVC	PFX_PRG_CODE(4,0),PRG_SAVE_CTX_CODE(0) \
	MVC	PFX_PRG_TRANS(4,0),PRG_SAVE_CTX_TRANS(0) \
.SkipRestore: \
	.drop	r15
#else
#define TRACE_PC
#endif

/* Program Interruption */
	.global ProgramCheck
ProgramCheck:
	SAVE_REGS (PC_INTERRUPT_BASE)
	TRACE_PC
	EXCEPTION_PROLOG (PC_INTERRUPT_BASE, PROG_PSW_OLD)
	EXCEPTION_CALL_C (ProgramCheckException)
	EXCEPTION_EPILOG (PC_INTERRUPT_BASE)

/* External */
	.global External
External:
	SAVE_REGS (INTERRUPT_BASE)
	EXCEPTION_TRACE (EXT_TRC_ID, PFX_EXT_SIG, PFX_EXT_CODE, EXTERN_PSW_OLD)
	EXCEPTION_PROLOG (INTERRUPT_BASE, EXTERN_PSW_OLD)
	EXCEPTION_CALL_C (ExternalException)
	EXCEPTION_EPILOG (INTERRUPT_BASE)

/* Input Output */
	.global InputOutput
InputOutput:
	SAVE_REGS (INTERRUPT_BASE)
	EXCEPTION_TRACE (IO_TRC_ID, PFX_SUBSYS_ID, PFX_IO_PARM, IO_PSW_OLD)
	EXCEPTION_PROLOG (INTERRUPT_BASE, IO_PSW_OLD)
	EXCEPTION_CALL_C (InputOutputException)
	EXCEPTION_EPILOG (INTERRUPT_BASE)

/* Literal Pool for this text page */
	.balign	8;			/* align literal pool */
	.ltorg;				/* dump literal pool */

/* ===================================================================== */
/* Miscellaneous stuff in the data segment */
/* p2align 12 aligns to a page boundary (2^12) */

	.data
	.p2align 12
	.globl	sdata
sdata:
	.globl	empty_zero_page
empty_zero_page:
	.space	4096

/*
 * swapper_pg_dir is the to be the virtual-mem page directory
 * table associated with the init process.  To map 2GB, it must
 * two pages in size.
 */
	.p2align 12
	.globl	swapper_pg_dir
swapper_pg_dir:
	.space	8192

/*
 * This space gets a copy of optional info passed to us by the bootstrap
 * Used to pass parameters into the kernel like root=/dev/sda1, etc.
 * Always the third page past start of .data, so bootloader can find it.
 * Default to /dev/ram; bootloader will clobber the value here.
 */
	.globl	cmd_line
cmd_line:
	.ascii "root=/dev/ram"
	.space	512

/* ===================================================================== */
/*
 * The syscall table is in the text section, making it RO.
 * The syscall table should match the table in <asm/unistd.h>.
 */
	.text
	.align 4
	.globl	sys_call_table
sys_call_table:
	.long sys_ni_syscall	/* 0  -  old "setup()" system call */
	.long sys_exit
	.long i370_sys_fork
	.long sys_read
	.long sys_write
	.long sys_open		/* 5 */
	.long sys_close
	.long sys_waitpid
	.long sys_creat
	.long sys_link
	.long sys_unlink	/* 10 */
	.long i370_sys_execve
	.long sys_chdir
	.long sys_time
	.long sys_mknod
	.long sys_chmod		/* 15 */
	.long sys_lchown
	.long sys_ni_syscall			/* old break syscall holder */
	.long sys_newstat
	.long sys_lseek
	.long sys_getpid	/* 20 */
	.long sys_mount
	.long sys_oldumount
	.long sys_setuid
	.long sys_getuid
	.long sys_stime		/* 25 */
	.long i370_sys_ptrace
	.long sys_alarm
	.long sys_fstat				/* old fstat call */
	.long i370_sys_pause
	.long sys_utime		/* 30 */
	.long sys_ni_syscall			/* old stty syscall holder */
	.long sys_ni_syscall			/* old gtty syscall holder */
	.long sys_access
	.long sys_nice
	.long sys_ni_syscall	/* 35 */	/* old ftime syscall holder */
	.long sys_sync
	.long sys_kill
	.long sys_rename
	.long sys_mkdir
	.long sys_rmdir		/* 40 */
	.long sys_dup
	.long i370_sys_pipe
	.long sys_times
	.long sys_ni_syscall			/* old prof syscall holder */
	.long sys_brk		/* 45 */
	.long sys_setgid
	.long sys_getgid
	.long sys_signal
	.long sys_geteuid
	.long sys_getegid	/* 50 */
	.long sys_acct
	.long sys_umount			/* recycled never used phys() */
	.long sys_ni_syscall			/* old lock syscall holder */
	.long sys_ioctl
	.long sys_fcntl		/* 55 */
	.long sys_ni_syscall			/* old mpx syscall holder */
	.long sys_setpgid
	.long sys_ni_syscall			/* old ulimit syscall holder */
	.long i370_sys_olduname
	.long sys_umask		/* 60 */
	.long sys_chroot
	.long sys_ustat
	.long sys_dup2
	.long sys_getppid
	.long sys_getpgrp	/* 65 */
	.long sys_setsid
	.long i370_sys_sigaction
	.long sys_sgetmask
	.long sys_ssetmask
	.long sys_setreuid	/* 70 */
	.long sys_setregid
	.long i370_sys_sigsuspend
	.long sys_sigpending
	.long sys_sethostname
	.long sys_setrlimit	/* 75 */
	.long sys_getrlimit
	.long sys_getrusage
	.long sys_gettimeofday
	.long sys_settimeofday
	.long sys_getgroups	/* 80 */
	.long sys_setgroups
	.long sys_select
	.long sys_symlink
	.long sys_newlstat
	.long sys_readlink	/* 85 */
	.long sys_uselib
	.long sys_swapon
	.long sys_reboot
	.long old_readdir
	.long i370_sys_mmap	/* 90 */
	.long sys_munmap
	.long sys_truncate
	.long sys_ftruncate
	.long sys_fchmod
	.long sys_fchown	/* 95 */
	.long sys_getpriority
	.long sys_setpriority
	.long sys_ni_syscall			/* old profil syscall holder */
	.long sys_statfs
	.long sys_fstatfs	/* 100 */
	.long i370_sys_ioperm
	.long sys_socketcall
	.long sys_syslog
	.long sys_setitimer
	.long sys_getitimer	/* 105 */
	.long sys_newstat
	.long sys_newlstat
	.long sys_newfstat
	.long i370_sys_uname			/* old uname */
	.long i370_sys_iopl	/* 110 */
	.long sys_vhangup
	.long i370_sys_idle
	.long i370_sys_vm86
	.long sys_wait4
	.long sys_swapoff	/* 115 */
	.long sys_sysinfo
	.long i370_sys_ipc
	.long sys_fsync
	.long i370_sys_sigreturn
	.long i370_sys_clone	/* 120 */
	.long sys_setdomainname
	.long sys_newuname
	.long i370_sys_modify_ldt
	.long sys_adjtimex
	.long sys_mprotect	/* 125 */
	.long sys_sigprocmask
	.long sys_create_module
	.long sys_init_module
	.long sys_delete_module
	.long sys_get_kernel_syms /* 130 */
	.long sys_quotactl
	.long sys_getpgid
	.long sys_fchdir
	.long sys_bdflush
	.long sys_sysfs		/* 135 */
	.long sys_personality
	.long sys_ni_syscall	/* for afs_syscall */
	.long sys_setfsuid
	.long sys_setfsgid
	.long sys_llseek	/* 140 */
	.long sys_getdents
	.long sys_select
	.long sys_flock
	.long sys_msync
	.long sys_readv		/* 145 */
	.long sys_writev
	.long sys_getsid
	.long sys_fdatasync
	.long sys_sysctl
	.long sys_mlock		/* 150 */
	.long sys_munlock
	.long sys_mlockall
	.long sys_munlockall
	.long sys_sched_setparam
	.long sys_sched_getparam	/* 155 */
	.long sys_sched_setscheduler
	.long sys_sched_getscheduler
	.long sys_sched_yield
	.long sys_sched_get_priority_max
	.long sys_sched_get_priority_min  /* 160 */
	.long sys_sched_rr_get_interval
	.long sys_nanosleep
	.long sys_mremap
	.long sys_setresuid
	.long sys_getresuid	/* 165 */
	.long sys_query_module
	.long sys_poll
	.long sys_nfsservctl
	.long sys_setresgid
	.long sys_getresgid	/* 170 */
	.long sys_prctl
	.long i370_sys_rt_sigreturn
	.long sys_rt_sigaction
	.long sys_rt_sigprocmask
	.long sys_rt_sigpending	/* 175 */
	.long sys_rt_sigtimedwait
	.long sys_rt_sigqueueinfo
	.long i370_sys_rt_sigsuspend
	.long sys_pread
	.long sys_pwrite	/* 180 */
	.long sys_chown
	.long sys_getcwd
	.long sys_capget
	.long sys_capset
	.long i370_sys_sigaltstack	/* 185 */
	.long sys_sendfile
	.long sys_ni_syscall		/* streams1 */
	.long sys_ni_syscall		/* streams2 */
	.space (256-NR_syscalls)*4,0xd0


/* =========================== end of file ============================== */
