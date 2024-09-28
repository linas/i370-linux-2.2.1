#ifndef __I370_SYSTEM_H
#define __I370_SYSTEM_H

#include <linux/kdev_t.h>
#include <asm/processor.h>
#include <asm/atomic.h>

/*
 * Memory barrier.
 * The BCR 15,0 (sync) instruction guarantees that all memory accesses initiated
 * by this processor have been performed (with respect to all other
 * mechanisms that access memory).
 */
#define mb()  __asm__ __volatile__ ("BCR 15,0" : : : "memory")
#define rmb()  __asm__ __volatile__ ("BCR 15,0" : : : "memory")
#define wmb()  __asm__ __volatile__ ("BCR 15,0" : : : "memory")

/* XXX some of these might be wrong */

extern __inline__ unsigned long __cli (void)
{
	unsigned char oldval;
	/* cli clears i/o & external int flags by AND'ing with mask */
	asm volatile ("STNSM    %0,0xfc" : "=m" (oldval) : : "memory");
	return oldval;
}

extern __inline__ unsigned long __sti (void)
{
	unsigned char oldval;
	/* sti enables i/o & external interrupts by OR'ing with mask */
	asm volatile ("STOSM    %0,0x3" : "=m" (oldval) : : "memory");
	return oldval;
}

extern __inline__ unsigned long __get_save_flags (void)
{
	unsigned char oldval;
	/* get the current psw mask bit by OR'ing with zero */
	asm volatile ("STOSM    %0,0x0" : "=m" (oldval) : : "memory" );
	return oldval;
}

#define __save_flags(flags)  ({flags = __get_save_flags(); })
#define __save_and_cli(flags)	({flags = __cli();})

extern __inline__ void __restore_flags(unsigned long flags)
{
	unsigned char newval = flags;
	/* Set System Mask clobbers all maskbits including PER */
	asm volatile ("SSM	%0" : : "m" (newval) : "memory");
}

#ifndef __SMP__

#define cli()	__cli()
#define sti()	__sti()
#define save_flags(flags)	__save_flags(flags)
#define restore_flags(flags)	__restore_flags(flags)
#define save_and_cli(flags)	__save_and_cli(flags)

#else /* __SMP__ */

extern void __global_cli(void);
extern void __global_sti(void);
extern unsigned long __global_save_flags(void);
extern void __global_restore_flags(unsigned long);
#define cli() __global_cli()
#define sti() __global_sti()
#define save_flags(x) ((x)=__global_save_flags())
#define restore_flags(x) __global_restore_flags(x)

#endif /* !__SMP__ */

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))

extern void *xchg_u64(void *ptr, unsigned long val);
extern void *xchg_u32(void *m, unsigned long val);

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 *
 * This only works if the compiler isn't horribly bad at optimizing.
 * gcc-2.5.8 reportedly can't handle this, but as that doesn't work
 * too well on the alpha anyway..
 */
extern void __xchg_called_with_bad_pointer(void);

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))
#define tas(ptr) (xchg((ptr),1))

static inline unsigned long __xchg(unsigned long x, void * ptr, int size)
{
	switch (size) {
		case 4:
			return (unsigned long )xchg_u32(ptr, x);
		case 8:
			return (unsigned long )xchg_u64(ptr, x);
	}
	__xchg_called_with_bad_pointer();
	return x;


}

extern inline void * xchg_ptr(void * m, void * val)
{
	return (void *) xchg_u32(m, (unsigned long) val);
}

extern void print_backtrace(unsigned long stack_pointer);
extern void show_regs(struct pt_regs * regs);
extern void giveup_fpu(void);
extern void smp_giveup_fpu(struct task_struct *);
extern void i370_setup_devices(void);


struct task_struct;
extern int i370_switch_to(struct task_struct *prev, struct task_struct *next);
#define switch_to i370_switch_to

extern void dump_regs(struct pt_regs *);

#endif
