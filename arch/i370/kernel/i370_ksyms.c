
#include <asm/ptrace.h>

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/types.h>

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

extern void transfer_to_handler(void);
extern void int_return(void);
extern void syscall_trace(void);
extern void do_IRQ(struct pt_regs *regs, int isfake);
extern void MachineCheckException(struct pt_regs *regs);
extern void AlignmentException(struct pt_regs *regs);
extern void ProgramCheckException(struct pt_regs *regs);
extern void SingleStepException(struct pt_regs *regs);


EXPORT_SYMBOL(syscall_trace);
EXPORT_SYMBOL(transfer_to_handler);
EXPORT_SYMBOL(int_return);

EXPORT_SYMBOL(MachineCheckException);
EXPORT_SYMBOL(AlignmentException);
EXPORT_SYMBOL(ProgramCheckException);
EXPORT_SYMBOL(SingleStepException);


EXPORT_SYMBOL(strlen);   
EXPORT_SYMBOL(strcmp);   
EXPORT_SYMBOL(memset);

EXPORT_SYMBOL(__copy_tofrom_user);

