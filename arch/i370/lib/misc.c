/*
 * arch/i370/lib/misc.c
 * Miscellaneous i370 utilities
 *
 * Copyright (c) 1999 Neale Fergusen
 */

#include <linux/kernel.h>
#include <asm/asm.h>

/* XXX might be useful to move abs to some header file and inline? */

int
abs (int j)
{
	int absj;

	__asm__ __volatile__ (
		"LPR     %0,%1"
		: "=r" (absj)
		: "r" (j)
		);

	return(absj);
}


/*
 * computes the checksum of a memory block at buff, length len,
 * and adds in "sum" (32-bit)
 *
 * returns a 32-bit number suitable for feeding into itself
 * or csum_tcpudp_magic
 *
 * this function must be called with even lengths, except
 * for the last fragment, which may be odd
 *
 * it's best to have buff aligned on a 32-bit boundary
 */

unsigned int
csum_partial(const unsigned char * buff, int len,
             unsigned int sum)
{
	unsigned int cksum = 0;

#ifdef CONFIG_CKSM
	/* The CKSM instruction exists only if the checksum
	 * facility is installed. */
	__asm__ __volatile__ (
	"	L       r8,%1;"
	"	L       r9,%2;"
	"	L       r1,%3;"
	"1:	CKSM    r1,r8;"
	"	BO      1b;"
	"	ST      r1,%0"
	: "=m" (cksum)
	: "m" (buff), "m" (len), "m" (sum)
	: "r1", "r8", "r9");
#else
	printk ("Error: csum_partial not implemented \n");
	i370_halt();
#endif /* CONFIG_CKSM */

	return (cksum);
}

/* XXX not implemented complete garbage ... */
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
unsigned int csum_partial_copy_generic(const char *src, char *dst,
                                              int len, unsigned int sum,
                                              int *src_err, int *dst_err)
{
	printk ("Error: csum_partial_copy_generic not implemented \n");
	i370_halt();
	return 0;
}

