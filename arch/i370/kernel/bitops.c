
/*
 * ESA/390 bitops
 * Copied from the PowerPC bitops; these are atomic and SMP-safe.
 * Copyright (c) 1999 Linas Vepstas
 *
 * XXX These should probably be moved to the header file and made inline.
 * They are here right now for debugging & convenience purposes.
 */
#include <linux/kernel.h>
#include <asm/bitops.h>

void set_bit(int nr, volatile void *addr)
{
	unsigned long oldval, newval;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%x, %p)\n", nr, addr);        

        __asm__ __volatile__("
1:      L	%0,%2;
	L	%1,%2;
	OR	%1,%3;
	CS	%0,%1,%2;
        BNE	1b"
        : "+r" (oldval), "+r" (newval), "+m" (*p)             
        : "r" (mask)
        : "memory");
}

void clear_bit(int nr, volatile void *addr)
{
	unsigned long oldval, newval;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%x, %p)\n", nr, addr);        

	mask = ~mask;  /* complement */

        __asm__ __volatile__("
1:      L	%0,%2;
	L	%1,%2;
	NR	%1,%3;
	CS	%0,%1,%2;
        BNE	1b"
        : "+r" (oldval), "+r" (newval), "+m" (*p)             
        : "r" (mask)
        : "memory");
}

void change_bit(int nr, volatile void *addr)
{
	unsigned long oldval, newval;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%x, %p)\n", nr, addr);        

        __asm__ __volatile__("
1:      L	%0,%2;
	L	%1,%2;
	XR	%1,%3;
	CS	%0,%1,%2;
        BNE	1b"
        : "+r" (oldval), "+r" (newval), "+m" (*p)             
        : "r" (mask)
        : "memory");
}

int test_and_set_bit(int nr, volatile void *addr)
{
	unsigned long oldval, newval;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%x, %p)\n", nr, addr);        

        __asm__ __volatile__("
1:      L	%0,%2;
	L	%1,%2;
	OR	%1,%3;
	CS	%0,%1,%2;
        BNE	1b"
        : "+r" (oldval), "+r" (newval), "+m" (*p)             
        : "r" (mask)
        : "memory");

	return  (oldval & mask);
}

int test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned long oldval, newval;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long maskcomp = ~mask;
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%x, %p)\n", nr, addr);        

        __asm__ __volatile__("
1:      L	%0,%2;
	L	%1,%2;
	NR	%1,%3;
	CS	%0,%1,%2;
        BNE	1b"
        : "+r" (oldval), "+r" (newval), "+m" (*p)             
        : "r" (maskcomp)
        : "memory");

	return  (oldval & mask);
}

int test_and_change_bit(int nr, volatile void *addr)
{
	unsigned long oldval, newval;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%x, %p)\n", nr, addr);        

        __asm__ __volatile__("
1:      L	%0,%2;
	L	%1,%2;
	XR	%1,%3;
	CS	%0,%1,%2;
        BNE	1b"
        : "+r" (oldval), "+r" (newval), "+m" (*p)             
        : "r" (mask)
        : "memory");

	return  (oldval & mask);
}

