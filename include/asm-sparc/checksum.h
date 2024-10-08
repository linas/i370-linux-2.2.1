/* $Id: checksum.h,v 1.1.1.1 1999/02/08 06:18:58 linas Exp $ */
#ifndef __SPARC_CHECKSUM_H
#define __SPARC_CHECKSUM_H

/*  checksum.h:  IP/UDP/TCP checksum routines on the Sparc.
 *
 *  Copyright(C) 1995 Linus Torvalds
 *  Copyright(C) 1995 Miguel de Icaza
 *  Copyright(C) 1996 David S. Miller
 *  Copyright(C) 1996 Eddie C. Dost
 *  Copyright(C) 1997 Jakub Jelinek
 *
 * derived from:
 *	Alpha checksum c-code
 *      ix86 inline assembly
 *      RFC1071 Computing the Internet Checksum
 */
 
#include <asm/uaccess.h>
#include <asm/cprefix.h>

/* computes the checksum of a memory block at buff, length len,
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
extern unsigned int csum_partial(unsigned char * buff, int len, unsigned int sum);

/* the same as csum_partial, but copies from fs:src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */

/* FIXME: Remove these two macros ASAP */
#define csum_partial_copy(src, dst, len, sum) \
 		       csum_partial_copy_nocheck(src,dst,len,sum)
#define csum_partial_copy_fromuser(s, d, l, w)  \
                         csum_partial_copy((char *) (s), (d), (l), (w))
  
extern unsigned int __csum_partial_copy_sparc_generic (const char *, char *);

extern __inline__ unsigned int 
csum_partial_copy_nocheck (const char *src, char *dst, int len, 
			   unsigned int sum)
{
	register unsigned int ret asm("o0") = (unsigned int)src;
	register char *d asm("o1") = dst;
	register int l asm("g1") = len;
	
	__asm__ __volatile__ ("
		call " C_LABEL_STR(__csum_partial_copy_sparc_generic) "
		 mov %4, %%g7
	" : "=r" (ret) : "0" (ret), "r" (d), "r" (l), "r" (sum) :
	"o1", "o2", "o3", "o4", "o5", "o7", "g1", "g2", "g3", "g4", "g5", "g7");
	return ret;
}

extern __inline__ unsigned int 
csum_partial_copy_from_user(const char *src, char *dst, int len, 
			    unsigned int sum, int *err)
  {
	if (!access_ok (VERIFY_READ, src, len)) {
		*err = -EFAULT;
		memset (dst, 0, len);
		return sum;
	} else {
		register unsigned int ret asm("o0") = (unsigned int)src;
		register char *d asm("o1") = dst;
		register int l asm("g1") = len;
		register unsigned int s asm("g7") = sum;

		__asm__ __volatile__ ("
		.section __ex_table,#alloc
		.align 4
		.word 1f,2
		.previous
1:
		call " C_LABEL_STR(__csum_partial_copy_sparc_generic) "
		 st %5, [%%sp + 64]
		" : "=r" (ret) : "0" (ret), "r" (d), "r" (l), "r" (s), "r" (err) :
		"o1", "o2", "o3", "o4", "o5", "o7", "g1", "g2", "g3", "g4", "g5", "g7");
		return ret;
	}
  }
  
extern __inline__ unsigned int 
csum_partial_copy_to_user(const char *src, char *dst, int len, 
			  unsigned int sum, int *err)
{
	if (!access_ok (VERIFY_WRITE, dst, len)) {
		*err = -EFAULT;
		return sum;
	} else {
		register unsigned int ret asm("o0") = (unsigned int)src;
		register char *d asm("o1") = dst;
		register int l asm("g1") = len;
		register unsigned int s asm("g7") = sum;

		__asm__ __volatile__ ("
		.section __ex_table,#alloc
		.align 4
		.word 1f,1
		.previous
1:
		call " C_LABEL_STR(__csum_partial_copy_sparc_generic) "
		 st %5, [%%sp + 64]
		" : "=r" (ret) : "0" (ret), "r" (d), "r" (l), "r" (s), "r" (err) :
		"o1", "o2", "o3", "o4", "o5", "o7", "g1", "g2", "g3", "g4", "g5", "g7");
		return ret;
	}
}
  
/* ihl is always 5 or greater, almost always is 5, and iph is word aligned
 * the majority of the time.
 */
extern __inline__ unsigned short ip_fast_csum(__const__ unsigned char *iph,
					      unsigned int ihl)
{
	unsigned short sum;

	/* Note: We must read %2 before we touch %0 for the first time,
	 *       because GCC can legitimately use the same register for
	 *       both operands.
	 */
	__asm__ __volatile__("sub\t%2, 4, %%g4\n\t"
			     "ld\t[%1 + 0x00], %0\n\t"
			     "ld\t[%1 + 0x04], %%g2\n\t"
			     "ld\t[%1 + 0x08], %%g3\n\t"
			     "addcc\t%%g2, %0, %0\n\t"
			     "addxcc\t%%g3, %0, %0\n\t"
			     "ld\t[%1 + 0x0c], %%g2\n\t"
			     "ld\t[%1 + 0x10], %%g3\n\t"
			     "addxcc\t%%g2, %0, %0\n\t"
			     "addx\t%0, %%g0, %0\n"
			     "1:\taddcc\t%%g3, %0, %0\n\t"
			     "add\t%1, 4, %1\n\t"
			     "addxcc\t%0, %%g0, %0\n\t"
			     "subcc\t%%g4, 1, %%g4\n\t"
			     "be,a\t2f\n\t"
			     "sll\t%0, 16, %%g2\n\t"
			     "b\t1b\n\t"
			     "ld\t[%1 + 0x10], %%g3\n"
			     "2:\taddcc\t%0, %%g2, %%g2\n\t"
			     "srl\t%%g2, 16, %0\n\t"
			     "addx\t%0, %%g0, %0\n\t"
			     "xnor\t%%g0, %0, %0"
			     : "=r" (sum), "=&r" (iph)
			     : "r" (ihl), "1" (iph)
			     : "g2", "g3", "g4", "cc");
	return sum;
}

/* Fold a partial checksum without adding pseudo headers. */
extern __inline__ unsigned int csum_fold(unsigned int sum)
{
	unsigned int tmp;

	__asm__ __volatile__("addcc\t%0, %1, %1\n\t"
			     "srl\t%1, 16, %1\n\t"
			     "addx\t%1, %%g0, %1\n\t"
			     "xnor\t%%g0, %1, %0"
			     : "=&r" (sum), "=r" (tmp)
			     : "0" (sum), "1" (sum<<16)
			     : "cc");
	return sum;
}

extern __inline__ unsigned long csum_tcpudp_nofold(unsigned long saddr,
						   unsigned long daddr,
						   unsigned int len,
						   unsigned short proto,
						   unsigned int sum)
{
	__asm__ __volatile__("addcc\t%1, %0, %0\n\t"
			     "addxcc\t%2, %0, %0\n\t"
			     "addxcc\t%3, %0, %0\n\t"
			     "addx\t%0, %%g0, %0\n\t"
			     : "=r" (sum), "=r" (saddr)
			     : "r" (daddr), "r" ((proto<<16)+len), "0" (sum),
			       "1" (saddr)
			     : "cc");
	return sum;
}

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */
static inline unsigned short int csum_tcpudp_magic(unsigned long saddr,
						   unsigned long daddr,
						   unsigned short len,
						   unsigned short proto,
						   unsigned int sum) 
{
	return csum_fold(csum_tcpudp_nofold(saddr,daddr,len,proto,sum));
}

#define _HAVE_ARCH_IPV6_CSUM

static __inline__ unsigned short int csum_ipv6_magic(struct in6_addr *saddr,
						     struct in6_addr *daddr,
						     __u16 len,
						     unsigned short proto,
						     unsigned int sum) 
{
	__asm__ __volatile__ ("
		addcc	%3, %4, %%g4
		addxcc	%5, %%g4, %%g4
		ld	[%2 + 0x0c], %%g2
		ld	[%2 + 0x08], %%g3
		addxcc	%%g2, %%g4, %%g4
		ld	[%2 + 0x04], %%g2
		addxcc	%%g3, %%g4, %%g4
		ld	[%2 + 0x00], %%g3
		addxcc	%%g2, %%g4, %%g4
		ld	[%1 + 0x0c], %%g2
		addxcc	%%g3, %%g4, %%g4
		ld	[%1 + 0x08], %%g3
		addxcc	%%g2, %%g4, %%g4
		ld	[%1 + 0x04], %%g2
		addxcc	%%g3, %%g4, %%g4
		ld	[%1 + 0x00], %%g3
		addxcc	%%g2, %%g4, %%g4
		addxcc	%%g3, %%g4, %0
		addx	0, %0, %0
		"
		: "=&r" (sum)
		: "r" (saddr), "r" (daddr), 
		  "r"(htonl((__u32) (len))), "r"(htonl(proto)), "r"(sum)
		: "g2", "g3", "g4", "cc");

	return csum_fold(sum);
}

/* this routine is used for miscellaneous IP-like checksums, mainly in icmp.c */
extern __inline__ unsigned short ip_compute_csum(unsigned char * buff, int len)
{
	return csum_fold(csum_partial(buff, len, 0));
}

#endif /* !(__SPARC_CHECKSUM_H) */
