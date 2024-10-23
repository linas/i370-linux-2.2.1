#ifndef _PPC_CHECKSUM_H
#define _PPC_CHECKSUM_H


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
extern unsigned int csum_partial(const unsigned char * buff, int len,
				 unsigned int sum);

extern unsigned int csum_partial_copy_from_user(const char *src,
             char *dst, int len, unsigned int sum, int *errp);

/*
 * the same as csum_partial, but copies from src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */
unsigned int csum_partial_copy(const char *src, char *dst, int len, unsigned int sum);

unsigned int csum_partial_copy_nocheck(const char *src, char *dst, int len, unsigned int sum);

/*
 * turns a 32-bit partial checksum (e.g. from csum_partial) into a
 * 1's complement 16-bit checksum.
 */
static inline unsigned int csum_fold(unsigned int sum)
{
	unsigned int tmp;

	/* swap the two 16-bit halves of sum */
	//	__asm__("rlwinm %0,%1,16,0,31" : "=r" (tmp) : "r" (sum));
	/* if there is a carry from adding the two 16-bit halves,
	   it will carry from the lower half into the upper half,
	   giving us the correct sum in the upper half. */
	sum = ~(sum + tmp) >> 16;
	return sum;

	// Alternate implementation
	// sum = (sum & 0xffff) + (sum >> 16);
	// sum = (sum & 0xffff) + (sum >> 16);
	// return sum;
}

/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */
static inline unsigned short ip_compute_csum(unsigned char * buff, int len)
{
	return csum_fold(csum_partial(buff, len, 0));
}

/*
 * FIXME: I swiped this one from the sparc and made minor modifications.
 * It may not be correct.  -- Cort
 */
static inline unsigned long csum_tcpudp_nofold(unsigned long saddr,
						   unsigned long daddr,
						   unsigned short len,
						   unsigned short proto,
						   unsigned int sum)
{
  // __asm__("
  //	addc %0,%0,%1
  //	adde %0,%0,%2
  //	adde %0,%0,%3
  //	addze %0,%0
  //	"
  //	: "=r" (sum)
  //	: "r" (daddr), "r"(saddr), "r"((proto<<16)+len), "0"(sum));
    return sum;
}

/*
 * This is a version of ip_compute_csum() optimized for IP headers,
 * which always checksum on 4 octet boundaries.  ihl is the number
 * of 32-bit words and is always >= 5.
 */
extern unsigned short ip_fast_csum(unsigned char * iph, unsigned int ihl);

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */
extern unsigned short csum_tcpudp_magic(unsigned long saddr,
					unsigned long daddr,
					unsigned short len,
					unsigned short proto,
					unsigned int sum);

#endif
