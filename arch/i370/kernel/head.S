/*
 * Implementation of startup code and FLIH entry/exit for ESA/390
 * Copyright (c) 1999 Linas Vepstas (linas@linas.org)
 *
 * Note: XXX hack alert: the current implementation of gcc/egcs
 * defines a defacto ESA/390 ELF ABI that has numerous faults.
 * (see the notes with the compiler for details).  The most serious
 * of these is how it handles address literals (which show up in
 * the text segment, instead of the data segment as they should)
 * This should be fixed,
 * however, if/when this is done, the code below will require
 * major tear-up.  Sorry, my fault, but so it goes. --linas
 */

#include <asm/errno.h>
#include <asm/processor.h>

/* Stop the machine. Handy for debugging purposes, but also
 * used in a few "it can't possibly happen" situations.
 */
#define HALT(sav)							\
	SPKA    sav(0);			/* Ensure we can use lo core */ \
	L	r1,=X'000A0000';	/* disabled wait             */	\
	ST	r1,sav(,0);						\
	L	r1,=X'00000210';	/* must be any valid address */	\
	ST	r1,sav+4(,0);						\
	LPSW	sav(0);			/* oops halt the machine     */

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

/* start of text section */
        .text
        .globl  _stext
_stext:
	.globl	_start
_start:
	STM	r0,r15,0x200(0)		/* command line passed in regs    */

	BASR	r15,0			/* load r15 with PSW              */
	.using	.,r15			/* use r15 as base register       */

	/* make sure AMODE is 31, interrupts disabled                     */
	L	r1,=X'00080000'		/* everything disabled            */
	ST	r1,INTERRUPT_BASE(,0)	/* store into prefex page         */
	L	r1,=A(_cinit)		/* jump to cinit next             */
	O	r1,=X'80000000'		/* AMODE 31                       */
	ST	r1,INTERRUPT_BASE+4(,0)	/* store into prefix page         */
	LPSW	INTERRUPT_BASE(0)	/* jump to label                  */
	
	/* perform initialization to call C routines                      */
_cinit:
	L	sp,=A(init_ksp)		/* load addr of stackptr into r13 */
	L	sp,0(,sp)		/* load address of stack into r13 */
	LA	sp,7(,sp)		/* doubleword align               */
	SRL	sp,3
	SLL	sp,3
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
 * sav-16  old interrupt mask
 * sav-8   offset from struct task_struct to sav area
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

/* these defines map the struct _i370_interrupt_state_s in <asm/ptrace.h> */
#define IR_PSW		0x0
#define IR_R11		0x8
#define IR_R12		0xc
#define IR_R13		0x10
#define IR_R14		0x14
#define IR_R15		0x18
#define IR_R0		0x1c
#define IR_R1		0x20
#define IR_R2		0x24
#define IR_R3		0x28	
#define IR_R4		0x2c

#define IR_R5		0x30
#define IR_R6		0x34
#define IR_R7		0x38
#define IR_R8		0x3c
#define IR_R9		0x40
#define IR_R10		0x44

#define IR_CR0		0x48
#define IR_CR1		0x4c
#define IR_OLDREGS	0x50

/* -------------------------------------------------------- */

/* stack related offsets; have nothing to do with the above */
#define STACK_SIZE	0x94	/* stack size */
#define ARGS_BASE	STACK_SIZE
#define ARGS_SIZE	0x4	/* args size */
#define FRAME_BASE	STACK_SIZE + ARGS_SIZE
#define FRAME_SIZE	0x60	/* frame size */
#define SF_SIZE		STACK_SIZE + ARGS_SIZE + FRAME_SIZE
#define SF_SIZE_LIT	=XL4'000000F8'

/* location of saved registers on stack */
#define ARGP		ARGS_BASE
#define SAVED_PSW	FRAME_BASE + IR_PSW
#define SAVED_R11	FRAME_BASE + IR_R11
#define SAVED_R12	FRAME_BASE + IR_R12
#define SAVED_R13	FRAME_BASE + IR_R13
#define SAVED_R14	FRAME_BASE + IR_R14
#define SAVED_R15	FRAME_BASE + IR_R15
#define SAVED_R0	FRAME_BASE + IR_R0
#define SAVED_R1	FRAME_BASE + IR_R1
#define SAVED_R2	FRAME_BASE + IR_R2

#define SAVED_R3	FRAME_BASE + IR_R3
#define SAVED_R4	FRAME_BASE + IR_R4
#define SAVED_R5	FRAME_BASE + IR_R5
#define SAVED_R6	FRAME_BASE + IR_R6
#define SAVED_R7	FRAME_BASE + IR_R7
#define SAVED_R8	FRAME_BASE + IR_R8
#define SAVED_R9	FRAME_BASE + IR_R9
#define SAVED_R10	FRAME_BASE + IR_R10

#define SAVED_OLDREGS	FRAME_BASE + IR_OLDREGS

#define SAVE_REGS(sav) 							\
	STM	r11,r4,sav+IR_R11(0);	/* save gprs to low mem */	\
	STM	r5,r10,sav+IR_R5(0);	/* for debugging only, remove later xxx */


#define EXCEPTION_PROLOG(sav,oldpsw) 					\
	BASR	r15,0;			/* load r15 with PSW         */	\
	.using	.,r15;			/* use r15 as base register  */	\
	L	r11,=V(current);	/* addr of current pointer   */	\
	L	r11,0(,r11);		/* addr of current process   */	\
	/* interrupted from user space or kernel space ?             */	\
	L	r1,oldpsw(,0);		/* get old psw flags into r1 */	\
	N	r1,=X'00010000';	/* is in problem state?      */	\
	BZ	1f;			/* sp is already ok if kern  */	\
	/* interrupted user proc, so need to find a kernel stack     */	\
	LA	sp,0(,r11);		/* addr of curr proc in r13  */	\
	A	sp,sav-4(,0);		/* add offset to ksp         */	\
	L	sp,0(,sp);		/* load kernel sp            */	\
1:					/* branch here if kernel     */	\
	/* we have a valid stack pointer; build a stackframe         */	\
	SL	sp,SF_SIZE_LIT;		/* subtract stackframe size  */	\
	/* move saved regs out of prefix page to stackframe          */	\
	MVC	SAVED_PSW(8,sp),oldpsw(0); /* save old psw on stack  */	\
	MVC	SAVED_R11(64,sp),sav+IR_R11(0);	/* 16 regs=64 bytes  */	\
	/* chain the interrupt state pointer into thread_struct      */	\
	LA	r1,0(,r11);		/* addr of curr proc in r1   */	\
	A	r1,sav-8(,0);		/* add offset to regs ptr    */	\
	L	r2,0(,r1);		/* load regs ptr             */	\
	ST	r2,SAVED_OLDREGS(,sp);	/* save regs ptr to stack    */	\
	LA	r2,SAVED_PSW(,sp);	/* put addr of newregs in r2 */	\
	ST	r2,0(,r1);		/* update regs ptr in proc   */	\
	

#define EXCEPTION_CALL_C(handler)  					\
	L	r15,=A(handler);	/* load address of C routine */	\
	LA	r2,SAVED_PSW(,sp);	/* ptr to saved psw, regs    */	\
	ST	r2,ARGP(,sp);		/* ptr to arguments on stack */	\
	LA	r2,ARGP(,sp);		/* pointer to arguments      */	\
	LA	r1,SAVED_PSW(,sp);	/* pointer to returned value */	\
	BASR	lr,r15;			/* jump to C routine         */	\
	.using	.,lr;			/* use r14 to the end        */	\


#define EXCEPTION_UNCHAIN(sav)	 					\
	/* chain the interrupt state pointer into thread_struct      */	\
	L	r11,=V(current);	/* addr of current pointer   */	\
	L	r11,0(,r11);		/* addr of current process   */	\
	A	r11,sav-8(,0);		/* add offset to regs ptr    */	\
	L	r2,SAVED_OLDREGS(,sp);	/* get old ptr from stack    */	\
	ST	r2,0(,r11);		/* restore regs ptr in proc  */


#define EXCEPTION_RETURN(sav)  						\
	/* get old PSW, restore registers, return from interrupt     */	\
	MVC	sav(8,0),SAVED_PSW(sp); /* restore old psw frm stack */	\
	LM	r11,r12,SAVED_R11(sp);	/* restore r11, r12          */	\
	LM	r14,r4,SAVED_R14(sp);	/* restore r14 to r4         */	\
	L	r13,SAVED_R13(sp);	/* restore r13               */	\
	LPSW	sav(0);			/* return from interrupt     */


#define EXCEPTION_EPILOG(sav)  						\
	EXCEPTION_UNCHAIN(sav)						\
	EXCEPTION_RETURN(sav)

/* areas used in the prefix page used for tracing */
#define	PFX_TRACE	0xd00

/* id numbers not addresses */
#define	IPL_TRC_ID	0x100
#define	MCH_TRC_ID	0x200
#define	SVC_TRC_ID	0x300
#define	PRG_TRC_ID	0x400
#define EXT_TRC_ID	0x500
#define IO_TRC_ID	0x600

#define EXCEPTION_TRACE(trc_id,subid,irpt_code,old_psw)			\
	SLR	r0,r0;			/* Not Used */			\
	LA	r1,trc_id(0);		/* Trace id */			\
	ST	r1,PFX_TRACE(0);	/* Store in PFX */		\
	L	r1,subid(0);		/* subchannel id */		\
	L	r2,irpt_code(0);	/* Get Irpt code */		\
	LM	r3,r4,old_psw(0);	/* Get Old PSW */		\
	TRACE	r0,r4,PFX_TRACE(0);	/* Trace Wondrous event */

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
#define NR_syscalls	188
	.global SupervisorCall
SupervisorCall:
	SAVE_REGS (INTERRUPT_BASE)
	EXCEPTION_TRACE (SVC_TRC_ID, 0, PFX_SVC_CODE, SVC_PSW_OLD)
	EXCEPTION_PROLOG (INTERRUPT_BASE,  SVC_PSW_OLD) 

	SLR	r2,r2			/* put zero in r2 */
	LH	r2,138(,0)		/* load interruption code from pfx */
	LTR	r2,r2			/* is r2 zero ? */
	BZ	3f			/* branch if zero */
	HALT (INTERRUPT_BASE)		/* stop the machine (debug) XXX */
3:
	L	r1,SAVED_R1(sp)		/* get syscall number */
	LA	r2,NR_syscalls(,0)	/* max of NR_syscalls */
	CR	r1,r2			/* syscall must be less than NR */
	BL	4f			/* valid syscall ? */
	LA	r2,ENOSYSCALL(,0)	/* load r2 with errno */
	ST	r2,SAVED_R15(,sp)	/* return r15 to caller */
	ST	r1,SAVED_R1(,sp)	/* return r1 to caller */
	EXCEPTION_EPILOG (INTERRUPT_BASE)	/* return from interrupt */
4:
	L	r2,=A(sys_call_table)	/* load address of sys call table */
	SLL	r1,2			/* 4 bytes per word ... */
	L	r15,0(r2,r1)		/* load address of C routine */
	STM     r5,r10,SAVED_R5(sp)	/* sys call args in regs */
	LA	r2,SAVED_R5(,sp)	/* pointer to args on stack */
	LA	r1,ARGP(,sp)		/* pointer to returned value */
	BASR	lr,r15			/* jump to C routine */
	.using	.,lr;			/* use r14 for remainder of the show */
	STNSM    INTERRUPT_BASE-16(0),0xfc;	/* disable interrupts */
	EXCEPTION_UNCHAIN (INTERRUPT_BASE)	/* unstack pointer chain */
	/* get old PSW, restore registers, return from interrupt */
	/* Note r15 is not restored, but is returned 'as-is' from syscall */
	MVC	INTERRUPT_BASE(8,0),SAVED_PSW(sp) /* restore old psw frm stack */
	LM	r11,r12,SAVED_R11(sp)	/* restore r11, r12 */	
	LM	r0,r4,SAVED_R0(sp)	/* restore r0 thru r4 */	
	L	r14,SAVED_R14(sp)	/* restore r14 */	
	L	r13,SAVED_R13(sp)	/* restore r13 */
	LPSW	INTERRUPT_BASE(0);	/* return from interrupt */


/* Program Interruption */
	.global ProgramCheck
ProgramCheck:
	SAVE_REGS (PC_INTERRUPT_BASE)
	BASR	r15,0
	.using	*,r15
	/*--------------------------------------------------------------*/
	/* If this is a trace table related program check then we'd     */
	/* better not generate a trace record or we could get ourselves */
	/* in a rather tight loop                                       */
	/*--------------------------------------------------------------*/
	L	r0,PFX_PRG_CODE(0)
	LA	r14,0x7f(0)
	NR	r0,r14
	LA	r14,PIC_TRACE_TABLE(0)
	CR	r0,r14
	BE	.SkipPCTrace
	EXCEPTION_TRACE (PRG_TRC_ID, PFX_PRG_TRANS, PFX_PRG_CODE, PROG_PSW_OLD)
.SkipPCTrace:
	.drop	r15
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
/* Miscellaneous stuff in the data segement */
/* p2align 12  aligns to a page boundry (2^12) */

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
 */	
/* 
 * XXX a hack to boot, for now
 * ramdisk size in KB, but ramdisk start in block number
 */
	.globl	cmd_line
cmd_line:
	/* .ascii "ramdisk_size=4096K root=/dev/ram" */
	.ascii "root=/dev/ram"
	.space	512

/* ===================================================================== */
/* The syscall table is in the text section; making it RO.
 *
 * The syscall table is copied from the powerpc code; it should match
 * the table in <asm/unistd.h> which was copied from powerpc code 
 * XXX might be a better deal to use the alpha syscall table... ??? XXX
 * XXX it needs to be hammered into shape for the i370
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
	.long sys_stat
	.long sys_lseek
	.long sys_getpid	/* 20 */
	.long sys_mount
	.long sys_oldumount
	.long sys_setuid
	.long sys_getuid
	.long sys_stime		/* 25 */
	.long i370_sys_ptrace
	.long sys_alarm
	.long sys_fstat
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
	.long sys_lstat
	.long sys_readlink	/* 85 */
	.long sys_uselib
	.long sys_swapon
	.long sys_reboot
	.long old_readdir
	.long i370_sys_mmap		/* 90 */
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
	.long i370_sys_uname
	.long i370_sys_iopl		/* 110 */
	.long sys_vhangup
	.long i370_sys_idle
	.long i370_sys_vm86
	.long sys_wait4
	.long sys_swapoff	/* 115 */
	.long sys_sysinfo
	.long i370_sys_ipc
	.long sys_fsync
	.long i370_sys_sigreturn
	.long i370_sys_clone		/* 120 */
	.long sys_setdomainname
	.long sys_newuname
	.long i370_sys_modify_ldt
	.long sys_adjtimex
	.long sys_mprotect	/* 125 */
	.long sys_sigprocmask
	.long sys_create_module
	.long sys_init_module
	.long sys_delete_module
	.long sys_get_kernel_syms	/* 130 */
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
