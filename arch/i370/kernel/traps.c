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
#include <asm/param.h>
#include <asm/ptrace.h>
#include <asm/signal.h>
#include <asm/system.h>

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

void instruction_dump (unsigned short *pc)
{
   int i;

   printk("Instruction DUMP:");
   for(i = -6; i < 12; i++)
      printk("%c%04x%c",i?' ':'<',pc[i],i?' ':'>');
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
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
      debugger(regs);
#endif
      // print_backtrace((unsigned long *)regs->gpr[0]);
      instruction_dump((unsigned short *)instruction_pointer(regs));
      panic("Exception in kernel psw.addr=%lx signal %d",regs->psw.addr,signr);
   }
   force_sig(signr, current);
}

psw_t
MachineCheckException(struct pt_regs *regs)
{
   psw_t retval;
   if ( !user_mode(regs) )
	{
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		if (debugger_fault_handler) {
			debugger_fault_handler(regs);
			return;
		}
#endif
		printk("Machine check in kernel mode.\n");
		printk("regs %p ",regs);
		show_regs(regs);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		debugger(regs);
#endif
		// print_backtrace((unsigned long *)regs->gpr[1]);
		instruction_dump((unsigned long *)regs->psw.addr);
		panic("machine check");
	}
	_exception(SIGSEGV, regs);	
   return retval;
}


psw_t
ProgramCheckException(struct pt_regs *regs)
{
   psw_t retval;
#ifdef JUNK_XXX
	if (regs->msr & 0x100000) {
		/* IEEE FP exception */
		_exception(SIGFPE, regs);
	} else if (regs->msr & 0x20000) {
		/* trap exception */
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
		if (debugger_bpt(regs))
			return;
#endif
		_exception(SIGTRAP, regs);
	} else {
		_exception(SIGILL, regs);
	}
#endif 
   return retval;
}

extern inline void i370_halt (void) 
{
	psw_t halt_psw;
	halt_psw.flags = HALT_PSW;
	halt_psw.addr = 0xffffffff;
	_lpsw (*((unsigned long long *) &halt_psw));
}
	

#define EX_HEADER(OLDPSW) 				\
	i370_interrupt_state_t state;			\
							\
	state.oldregs = current->tss.regs;		\
	current->tss.regs = &state;			\
							\
	/* move saved registers out of low page */	\
	state.irregs = saved_regs.irregs;		\
							\
	/* save old psw */				\
	state.psw = *((psw_t *) OLDPSW); 		\



#define EX_TRAILER {					\
	current->tss.regs = state.oldregs;		\
	return state;					\
}


i370_interrupt_state_t
RestartException(i370_interrupt_state_t saved_regs)
{
	EX_HEADER(IPL_PSW_OLD);

	printk ("restart exception\n");
	panic("machine check");

	EX_TRAILER;
}

i370_interrupt_state_t
SupervisorCallException (i370_interrupt_state_t saved_regs)
{
	unsigned long svc_code;
	EX_HEADER(SVC_PSW_OLD);

	/* get the interruption code */
	svc_code = *((unsigned long *) SVC_INT_CODE);

	/* real SVC's use an interruption code of zero,
	 * all other SVC codes are for debugging. */
	if (0 != (svc_code & 0xffff)) {
		panic ("bad svc\n");
		i370_halt();
	}
	printk ("svc exception\n");

	EX_TRAILER;
}

i370_interrupt_state_t
InputOutputException(i370_interrupt_state_t saved_regs)
{
	EX_HEADER(IO_PSW_OLD);

	printk ("io exception\n");

	EX_TRAILER;
}

i370_interrupt_state_t
ExternalException (i370_interrupt_state_t saved_regs)
{
	unsigned short code;
	unsigned long long ticko;

	EX_HEADER(EXTERN_PSW_OLD);

	/* get the interruption code */
	code = *((unsigned short *) EXT_INT_CODE);

	/* currently we only handle and expect clock interrupts */
	if ( EI_CLOCK_COMP != code) {
		printk ("unexpected external exception code=0x%x\n", code);
		panic ("unexpected external exception\n");
	}

	/* get and set the new clock value */
	/* clock ticks every 250 picoseconds (actually, every
	 * microsecond/4K). We want an interrupt every HZ of 
	 * a second */
	ticko = _stckc ();
	ticko += (1000000/HZ) << 12;
	_sckc (ticko);
	
	/* let Linux do its timer thing */
	do_timer (&state);

	EX_TRAILER;
}

/* ================================================================ */


void
StackOverflow(struct pt_regs *regs)
{
	// printk(KERN_CRIT "Kernel stack overflow in process %p, sp=%lx\n",
	//        current, regs->gpr[1]);
#if defined(CONFIG_XMON) || defined(CONFIG_KGDB)
	debugger(regs);
#endif
	show_regs(regs);
	// print_backtrace((unsigned long *)regs->gpr[1]);
	// instruction_dump((unsigned long *)regs->psw.addr);
	panic("kernel stack overflow");
}

/* ================================================================ */
/* wire in the interrupt vectors */

extern void SupervisorCall (void);
extern void External (void);

/*
 * we assume that we initialize traps while in real mode, i.e.
 * i.e. that the fixed storage locations (fsl) at low memory 
 * are where they're supposed to be.
 */

__initfunc(void trap_init(void))
{
	unsigned long long clock_reset;
	unsigned long *sz;
	psw_t psw;
	printk ("trap init");

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
	*(sz-2) = (unsigned long) &(((struct task_struct *) 0) ->tss.tca[0]);

#ifdef LATER
	// install the SVC handler
	psw.flags = PSW_VALID;        // disable all interupts
	psw.addr = ((unsigned long) SupervisorCall) | (1<<31); 
	*((psw_t *) SVC_PSW_NEW) = psw;
#endif

	// install the External Interrupt (clock) handler
	psw.flags = PSW_VALID;        // disable all interupts
	psw.addr = ((unsigned long) External) | (1<<31); 
	*((psw_t *) EXTERN_PSW_NEW) = psw;
}

/* ================================================================ */
#ifdef JUNK

unsigned long sys_call_table[__NR_syscalls];

init_sys_call_table ()
{
	sys_call_table[] = 

	sys_call_table [__NR_exit ] 	= sys_exit;	//	  1
	sys_call_table [__NR_fork ] 	= sys_fork;	//	  2
	sys_call_table [__NR_read ] 	= sys_read;	//	  3
	sys_call_table [__NR_write ] 	= sys_write;	//	  4
	sys_call_table [__NR_open ] 	= sys_open;	//	  5
	sys_call_table [__NR_close ] 	= sys_close;	//	  6
	sys_call_table [__NR_waitpid ] 	= sys_waitpid;	//	  7
	sys_call_table [__NR_creat ] 	= sys_creat;	//	  8
	sys_call_table [__NR_link ] 	= sys_link;	//	  9
	sys_call_table [__NR_unlink ] 	= sys_unlink;	//	 10
	sys_call_table [__NR_execve ] 	= sys_execve;	//	 11
	sys_call_table [__NR_chdir ] 	= sys_chdir;	//	 12
	sys_call_table [__NR_time ] 	= sys_time;	//	 13
	sys_call_table [__NR_mknod ] 	= sys_mknod;	//	 14
	sys_call_table [__NR_chmod ] 	= sys_chmod;	//	 15
	sys_call_table [__NR_lchown ] 	= sys_lchown;	//	 16
	sys_call_table [__NR_break ] 	= sys_break;	//	 17
	sys_call_table [__NR_oldstat ] 	= sys_oldstat;	//	 18
	sys_call_table [__NR_lseek ] 	= sys_lseek;	//	 19
	sys_call_table [__NR_getpid ] 	= sys_getpid;	//	 20
	sys_call_table [__NR_mount ] 	= sys_mount;	//	 21
	sys_call_table [__NR_umount ] 	= sys_umount;	//	 22
	sys_call_table [__NR_setuid ] 	= sys_setuid;	//	 23
	sys_call_table [__NR_getuid ] 	= sys_getuid;	//	 24
	sys_call_table [__NR_stime ] 	= sys_stime;	//	 25
	sys_call_table [__NR_ptrace ] 	= sys_ptrace;	//	 26
	sys_call_table [__NR_alarm ] 	= sys_alarm;	//	 27
	sys_call_table [__NR_oldfstat ]	= sys_oldfstat;	//	 28
	sys_call_table [__NR_pause ] 	= sys_pause;	//	 29
	sys_call_table [__NR_utime ] 	= sys_utime;	//	 30
	sys_call_table [__NR_stty ] 	= sys_stty;	//	 31
	sys_call_table [__NR_gtty ] 	= sys_gtty;	//	 32
	sys_call_table [__NR_access ] 	= sys_access;	//	 33
	sys_call_table [__NR_nice ] 	= sys_nice;	//	 34
	sys_call_table [__NR_ftime ] 	= sys_ftime;	//	 35
	sys_call_table [__NR_sync ] 	= sys_		//	 36
	sys_call_table [__NR_kill ] 	= sys_		//	 37
	sys_call_table [__NR_rename ] 	= sys_		//	 38
	sys_call_table [__NR_mkdir ] 	= sys_		//	 39
	sys_call_table [__NR_rmdir ] 	= sys_		//	 40
	sys_call_table [__NR_dup ] 	= sys_		//	 41
	sys_call_table [__NR_pipe ] 	= sys_		//	 42
	sys_call_table [__NR_times ] 	= sys_		//	 43
	sys_call_table [__NR_prof ] 	= sys_		//	 44
	sys_call_table [__NR_brk ] 	= sys_		//	 45
	sys_call_table [__NR_setgid ] 	= sys_		//	 46
	sys_call_table [__NR_getgid ] 	= sys_		//	 47
	sys_call_table [__NR_signal ] 	= sys_		//	 48
	sys_call_table [__NR_geteuid ] 	= sys_		//	 49
	sys_call_table [__NR_getegid ] 	= sys_		//	 50
	sys_call_table [__NR_acct ] 	= sys_		//	 51
	sys_call_table [__NR_umount2 ] 	= sys_		//	 52
	sys_call_table [__NR_lock ] 	= sys_		//	 53
	sys_call_table [__NR_ioctl ] 	= sys_		//	 54
	sys_call_table [__NR_fcntl ] 	= sys_		//	 55
	sys_call_table [__NR_mpx ] 	= sys_		//	 56
	sys_call_table [__NR_setpgid ] 	= sys_		//	 57
	sys_call_table [__NR_ulimit ] 	= sys_		//	 58
	sys_call_table [__NR_oldolduname ] 	= sys_		// 59
	sys_call_table [__NR_umask ] 	= sys_		//	 60
	sys_call_table [__NR_chroot ] 	= sys_		//	 61
	sys_call_table [__NR_ustat ] 	= sys_		//	 62
	sys_call_table [__NR_dup2 ] 	= sys_		//	 63
	sys_call_table [__NR_getppid ] 	= sys_		//	 64
	sys_call_table [__NR_getpgrp ] 	= sys_		//	 65
	sys_call_table [__NR_setsid ] 	= sys_		//	 66
	sys_call_table [__NR_sigaction ] 	= sys_		//	 67
	sys_call_table [__NR_sgetmask ] 	= sys_		//	 68
	sys_call_table [__NR_ssetmask ] 	= sys_		//	 69
	sys_call_table [__NR_setreuid ] 	= sys_		//	 70
	sys_call_table [__NR_setregid ] 	= sys_		//	 71
	sys_call_table [__NR_sigsuspend ] 	= sys_		//	 72
	sys_call_table [__NR_sigpending ] 	= sys_		//	 73
	sys_call_table [__NR_sethostname ] 	= sys_		// 74
	sys_call_table [__NR_setrlimit ] 	= sys_		//	 75
	sys_call_table [__NR_getrlimit ] 	= sys_		//	 76
	sys_call_table [__NR_getrusage ] 	= sys_		//	 77
	sys_call_table [__NR_gettimeofday ] 	= sys_		// 78
	sys_call_table [__NR_settimeofday ] 	= sys_		// 79
	sys_call_table [__NR_getgroups ] 	= sys_		//	 80
	sys_call_table [__NR_setgroups ] 	= sys_		//	 81
	sys_call_table [__NR_select ] 	= sys_		//	 82
	sys_call_table [__NR_symlink ] 	= sys_		//	 83
	sys_call_table [__NR_oldlstat ] 	= sys_		//	 84
	sys_call_table [__NR_readlink ] 	= sys_		//	 85
	sys_call_table [__NR_uselib ] 	= sys_		//	 86
	sys_call_table [__NR_swapon ] 	= sys_		//	 87
	sys_call_table [__NR_reboot ] 	= sys_		//	 88
	sys_call_table [__NR_readdir ] 	= sys_		//	 89
	sys_call_table [__NR_mmap ] 	= sys_		//	 90
	sys_call_table [__NR_munmap ] 	= sys_		//	 91
	sys_call_table [__NR_truncate ] 	= sys_		//	 92
	sys_call_table [__NR_ftruncate ] 	= sys_		//	 93
	sys_call_table [__NR_fchmod ] 	= sys_		//	 94
	sys_call_table [__NR_fchown ] 	= sys_		//	 95
	sys_call_table [__NR_getpriority ] 	= sys_		// 96
	sys_call_table [__NR_setpriority ] 	= sys_		// 97
	sys_call_table [__NR_profil ] 	= sys_		//	 98
	sys_call_table [__NR_statfs ] 	= sys_		//	 99
	sys_call_table [__NR_fstatfs ] 	= sys_		//	100
	sys_call_table [__NR_ioperm ] 	= sys_		//	101
	sys_call_table [__NR_socketcall ] 	= sys_		//	102
	sys_call_table [__NR_syslog ] 	= sys_		//	103
	sys_call_table [__NR_setitimer ] 	= sys_		//	104
	sys_call_table [__NR_getitimer ] 	= sys_		//	105
	sys_call_table [__NR_stat ] 	= sys_		//	106
	sys_call_table [__NR_lstat ] 	= sys_		//	107
	sys_call_table [__NR_fstat ] 	= sys_		//	108
	sys_call_table [__NR_olduname ] 	= sys_		//	109
	sys_call_table [__NR_iopl ] 	= sys_		//	110
	sys_call_table [__NR_vhangup ] 	= sys_		//	111
	sys_call_table [__NR_idle ] 	= sys_		//	112
	sys_call_table [__NR_vm86 ] 	= sys_		//	113
	sys_call_table [__NR_wait4 ] 	= sys_		//	114
	sys_call_table [__NR_swapoff ] 	= sys_		//	115
	sys_call_table [__NR_sysinfo ] 	= sys_		//	116
	sys_call_table [__NR_ipc ] 	= sys_		//	117
	sys_call_table [__NR_fsync ] 	= sys_		//	118
	sys_call_table [__NR_sigreturn ] 	= sys_		//	119
	sys_call_table [__NR_clone ] 	= sys_		//	120
	sys_call_table [__NR_setdomainname ] 	= sys_		//121
	sys_call_table [__NR_uname ] 	= sys_		//	122
	sys_call_table [__NR_modify_ldt ] 	= sys_		//	123
	sys_call_table [__NR_adjtimex ] 	= sys_		//	124
	sys_call_table [__NR_mprotect ] 	= sys_		//	125
	sys_call_table [__NR_sigprocmask ] 	= sys_		//126
	sys_call_table [__NR_create_module ] 	= sys_		//127
	sys_call_table [__NR_init_module ] 	= sys_		//128
	sys_call_table [__NR_delete_module ] 	= sys_		//129
	sys_call_table [__NR_get_kernel_syms ] 	= sys_		//130
	sys_call_table [__NR_quotactl ] 	= sys_		//	131
	sys_call_table [__NR_getpgid ] 	= sys_		//	132
	sys_call_table [__NR_fchdir ] 	= sys_		//	133
	sys_call_table [__NR_bdflush ] 	= sys_		//	134
	sys_call_table [__NR_sysfs ] 	= sys_		//	135
	sys_call_table [__NR_personality ] 	= sys_		//136
	sys_call_table [__NR_afs_syscall ] 	= sys_		//137 
	sys_call_table [__NR_setfsuid ] 	= sys_		//	138
	sys_call_table [__NR_setfsgid ] 	= sys_		//	139
	sys_call_table [__NR__llseek ] 	= sys_		//	140
	sys_call_table [__NR_getdents ] 	= sys_		//	141
	sys_call_table [__NR__newselect ] 	= sys_		//	142
	sys_call_table [__NR_flock ] 	= sys_		//	143
	sys_call_table [__NR_msync ] 	= sys_		//	144
	sys_call_table [__NR_readv ] 	= sys_		//	145
	sys_call_table [__NR_writev ] 	= sys_		//	146
	sys_call_table [__NR_getsid ] 	= sys_		//	147
	sys_call_table [__NR_fdatasync ] 	= sys_		//	148
	sys_call_table [__NR__sysctl ] 	= sys_		//	149
	sys_call_table [__NR_mlock ] 	= sys_		//	150
	sys_call_table [__NR_munlock ] 	= sys_		//	151
	sys_call_table [__NR_mlockall ] 	= sys_		//	152
	sys_call_table [__NR_munlockall ] 	= sys_		//	153
	sys_call_table [__NR_sched_setparam ] 	= sys_		//	154
	sys_call_table [__NR_sched_getparam ] 	= sys_		//	155
	sys_call_table [__NR_sched_setscheduler ] 	= sys_		//	156
	sys_call_table [__NR_sched_getscheduler ] 	= sys_		//	157
	sys_call_table [__NR_sched_yield ] 	= sys_		//	158
	sys_call_table [__NR_sched_get_priority_max ] 	= sys_		//159
	sys_call_table [__NR_sched_get_priority_min ] 	= sys_		//160
	sys_call_table [__NR_sched_rr_get_interval ] 	= sys_		//161
	sys_call_table [__NR_nanosleep ] 	= sys_		//	162
	sys_call_table [__NR_mremap ] 	= sys_		//	163
	sys_call_table [__NR_setresuid ] 	= sys_		//	164
	sys_call_table [__NR_getresuid ] 	= sys_		//	165
	sys_call_table [__NR_query_module ] 	= sys_		//166
	sys_call_table [__NR_poll ] 	= sys_		//	167
	sys_call_table [__NR_nfsservctl ] 	= sys_		//	168
	sys_call_table [__NR_setresgid ] 	= sys_		//	169
	sys_call_table [__NR_getresgid ] 	= sys_		//	170
	sys_call_table [__NR_prctl ] 	= sys_		//	171
	sys_call_table [__NR_rt_sigreturn ] 	= sys_		//172
	sys_call_table [__NR_rt_sigaction ] 	= sys_		//173
	sys_call_table [__NR_rt_sigprocmask ] 	= sys_		//174
	sys_call_table [__NR_rt_sigpending ] 	= sys_		//175
	sys_call_table [__NR_rt_sigtimedwait ] 	= sys_		//176
	sys_call_table [__NR_rt_sigqueueinfo ] 	= sys_		//177
	sys_call_table [__NR_rt_sigsuspend ] 	= sys_		//178
	sys_call_table [__NR_pread ] 	= sys_		//	179
	sys_call_table [__NR_pwrite ] 	= sys_		//	180
	sys_call_table [__NR_chown ] 	= sys_		//	181
	sys_call_table [__NR_getcwd ] 	= sys_		//	182
	sys_call_table [__NR_capget ] 	= sys_		//	183
	sys_call_table [__NR_capset ] 	= sys_		//	184
	sys_call_table [__NR_sigaltstack ] 	= sys_		//185
	sys_call_table [__NR_sendfile ] 	= sys_		//	186
	sys_call_table [__NR_getpmsg ] 	= sys_		//	187
	sys_call_table [__NR_putpmsg ] 	= sys_		//	188

}

#endif

/* ===================== END OF FILE =================================== */
