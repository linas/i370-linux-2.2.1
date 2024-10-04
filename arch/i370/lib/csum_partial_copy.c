/*
 * arch/i370/lib/csum_partial_copy.c
 * tcp packet checksumming.
 */

#include <asm/asm.h>
#include <asm/current.h>
#include <asm/system.h>

#include <linux/kernel.h>
#include <linux/sched.h>

/* 
 * Computes the checksum of a memory block at src, length len,
 * and adds in "sum" (32-bit), while copying the block to dst.
 * If an access exception occurs on src or dst, it stores -EFAULT
 * to *src_err or *dst_err respectively (if that pointer is not
 * NULL), and, for an error on src, zeroes the rest of dst. 
 *
 * Like csum_partial, this must be called with even lengths,
 * except for the last fragment.
 */
unsigned int
csum_partial_copy_from_user(const char *src, char *dst, int len,
             int sum, int *csum_err)
{
	printk("Error: csum_partial_copy_from_user not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 0;
}

unsigned int
csum_partial_copy (const char *src, char *dst, int len, unsigned int sum)
{
	printk("Error: csum_partial_copy not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 0;
}

unsigned int
csum_partial_copy_nocheck(const char *src, char *dst, int len, unsigned int sum)
{
	printk("Error: csum_partial_copy_nocheck not yet implemented\n");
	show_regs (current->tss.regs);
	print_backtrace (current->tss.regs->irregs.r13);
	i370_halt();
	return 0;
}
