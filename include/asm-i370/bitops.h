/*
 * bitops.h: Bit string operations on the i370
 */

#ifndef _I370_BITOPS_H
#define _I370_BITOPS_H

#include <asm/system.h>
#include <asm/byteorder.h>

extern void set_bit(int nr, volatile void *addr);
extern void clear_bit(int nr, volatile void *addr);
extern void change_bit(int nr, volatile void *addr);
extern int test_and_set_bit(int nr, volatile void *addr);
extern int test_and_clear_bit(int nr, volatile void *addr);
extern int test_and_change_bit(int nr, volatile void *addr);


/*
 * This routine doesn't need to be atomic.
 */
extern __inline__ unsigned long test_bit(int nr, __const__ volatile void *addr)
{
	__const__ unsigned int *p = (__const__ unsigned int *) addr;

	return (p[nr >> 5] >> (nr & 0x1f)) & 1UL;
}

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
extern __inline__ unsigned long ffz(unsigned long word)
{
        int k;

        word = ~word;
        k = 31;
        if (word & 0x0000ffff) { k -= 16; word <<= 16; }
        if (word & 0x00ff0000) { k -= 8;  word <<= 8;  }
        if (word & 0x0f000000) { k -= 4;  word <<= 4;  }
        if (word & 0x30000000) { k -= 2;  word <<= 2;  }
        if (word & 0x40000000) { k -= 1; }
        return k;
}


#ifdef __KERNEL__

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */

#define ffs(x) generic_ffs(x)

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */

#define hweight32(x) generic_hweight32(x)
#define hweight16(x) generic_hweight16(x)
#define hweight8(x) generic_hweight8(x)

#endif /* __KERNEL__ */

/*
 * This implementation of find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h.
 */
#define find_first_zero_bit(addr, size) \
	find_next_zero_bit((addr), (size), 0)

extern __inline__ unsigned long find_next_zero_bit(void * addr,
	unsigned long size, unsigned long offset)
{
	unsigned int * p = ((unsigned int *) addr) + (offset >> 5);
	unsigned int result = offset & ~31UL;
	unsigned int tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if (offset) {
		tmp = *p++;
		tmp |= ~0UL >> (32-offset);
		if (size < 32)
			goto found_first;
		if (tmp != ~0U)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while (size >= 32) {
		if ((tmp = *p++) != ~0U)
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = *p;
found_first:
	tmp |= ~0UL << size;
found_middle:
	return result + ffz(tmp);
}


#define _EXT2_HAVE_ASM_BITOPS_

#ifdef __KERNEL__
/*
 * test_and_{set,clear}_bit guarantee atomicity without
 * disabling interrupts.
 */
#define ext2_set_bit(nr, addr)		test_and_set_bit((nr) ^ 0x18, addr)
#define ext2_clear_bit(nr, addr)	test_and_clear_bit((nr) ^ 0x18, addr)

#else
extern __inline__ int ext2_set_bit(int nr, void * addr)
{
	int		mask;
	unsigned char	*ADDR = (unsigned char *) addr;
	int oldbit;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	oldbit = (*ADDR & mask) ? 1 : 0;
	*ADDR |= mask;
	return oldbit;
}

extern __inline__ int ext2_clear_bit(int nr, void * addr)
{
	int		mask;
	unsigned char	*ADDR = (unsigned char *) addr;
	int oldbit;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	oldbit = (*ADDR & mask) ? 1 : 0;
	*ADDR = *ADDR & ~mask;
	return oldbit;
}
#endif	/* __KERNEL__ */

extern __inline__ int ext2_test_bit(int nr, __const__ void * addr)
{
	__const__ unsigned char	*ADDR = (__const__ unsigned char *) addr;

	return (ADDR[nr >> 3] >> (nr & 7)) & 1;
}

/*
 * This implementation of ext2_find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h and modified for a big-endian machine.
 */

#define ext2_find_first_zero_bit(addr, size) \
        ext2_find_next_zero_bit((addr), (size), 0)

extern __inline__ unsigned long ext2_find_next_zero_bit(void *addr,
	unsigned long size, unsigned long offset)
{
	unsigned int *p = ((unsigned int *) addr) + (offset >> 5);
	unsigned int result = offset & ~31UL;
	unsigned int tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if (offset) {
		tmp = cpu_to_le32p(p++);
		tmp |= ~0UL >> (32-offset);
		if (size < 32)
			goto found_first;
		if (tmp != ~0U)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while (size >= 32) {
		if ((tmp = cpu_to_le32p(p++)) != ~0U)
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = cpu_to_le32p(p);
found_first:
	tmp |= ~0U << size;
found_middle:
	return result + ffz(tmp);
}

/* Bitmap functions for the minix filesystem.  */
#define minix_set_bit(nr,addr) ext2_set_bit(nr,addr)
#define minix_clear_bit(nr,addr) ext2_clear_bit(nr,addr)
#define minix_test_bit(nr,addr) ext2_test_bit(nr,addr)
#define minix_find_first_zero_bit(addr,size) ext2_find_first_zero_bit(addr,size)

#endif /* _I370_BITOPS_H */
