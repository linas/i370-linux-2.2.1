#ifndef _I370_IO_H
#define _I370_IO_H
/* XXX this is all wrong */

#include <linux/config.h>
#include <asm/page.h>
#include <asm/byteorder.h>

#define KERNELBASE	0xc0000000

#define SIO_CONFIG_RA	0x398
#define SIO_CONFIG_RD	0x399

#define IBM_HDD_LED       0x808
#define IBM_EQUIP_PRESENT 0x80c	
#define IBM_L2_STATUS     0x80d
#define IBM_L2_INVALIDATE 0x814
#define IBM_SYS_CTL       0x81c

#define SLOW_DOWN_IO

#define PREP_ISA_IO_BASE 	0x80000000
#define PREP_ISA_MEM_BASE 	0xd0000000
/*#define PREP_ISA_MEM_BASE 	0xc0000000*/
#define PREP_PCI_DRAM_OFFSET 	0x80000000

#define _IO_BASE        0
#define _ISA_MEM_BASE   0
#define PCI_DRAM_OFFSET 0x80000000

#define readb(addr) in_8((volatile unsigned char *)(addr))
#define writeb(b,addr) out_8((volatile unsigned char *)(addr), (b))
#define readw(addr) in_le16((volatile unsigned short *)(addr))
#define readl(addr) in_le32((volatile unsigned *)addr)
#define writew(b,addr) out_le16((volatile unsigned short *)(addr),(b))
#define writel(b,addr) out_le32((volatile unsigned *)(addr),(b))

#define insb(port, buf, ns)	_insb((unsigned char *)((port)+_IO_BASE), (buf), (ns))
#define outsb(port, buf, ns)	_outsb((unsigned char *)((port)+_IO_BASE), (buf), (ns))
#define insw(port, buf, ns)	_insw((unsigned short *)((port)+_IO_BASE), (buf), (ns))
#define outsw(port, buf, ns)	_outsw((unsigned short *)((port)+_IO_BASE), (buf), (ns))
#define insl(port, buf, nl)	_insl((unsigned long *)((port)+_IO_BASE), (buf), (nl))
#define outsl(port, buf, nl)	_outsl((unsigned long *)((port)+_IO_BASE), (buf), (nl))

#define inb(port)		in_8((unsigned char *)((port)+_IO_BASE))
#define outb(val, port)		out_8((unsigned char *)((port)+_IO_BASE), (val))
#define inw(port)		in_le16((unsigned short *)((port)+_IO_BASE))
#define outw(val, port)		out_le16((unsigned short *)((port)+_IO_BASE), (val))
#define inl(port)		in_le32((unsigned *)((port)+_IO_BASE))
#define outl(val, port)		out_le32((unsigned *)((port)+_IO_BASE), (val))

#define inb_p(port)		in_8((unsigned char *)((port)+_IO_BASE))
#define outb_p(val, port)	out_8((unsigned char *)((port)+_IO_BASE), (val))
#define inw_p(port)		in_le16((unsigned short *)((port)+_IO_BASE))
#define outw_p(val, port)	out_le16((unsigned short *)((port)+_IO_BASE), (val))
#define inl_p(port)		in_le32(((unsigned *)(port)+_IO_BASE))
#define outl_p(val, port)	out_le32((unsigned *)((port)+_IO_BASE), (val))

extern void _insb(volatile unsigned char *port, void *buf, int ns);
extern void _outsb(volatile unsigned char *port, const void *buf, int ns);
extern void _insw(volatile unsigned short *port, void *buf, int ns);
extern void _outsw(volatile unsigned short *port, const void *buf, int ns);
extern void _insl(volatile unsigned long *port, void *buf, int nl);
extern void _outsl(volatile unsigned long *port, const void *buf, int nl);

/*
 * The *_ns versions below don't do byte-swapping.
 */
#define insw_ns(port, buf, ns)	_insw_ns((unsigned short *)((port)+_IO_BASE), (buf), (ns))
#define outsw_ns(port, buf, ns)	_outsw_ns((unsigned short *)((port)+_IO_BASE), (buf), (ns))
#define insl_ns(port, buf, nl)	_insl_ns((unsigned long *)((port)+_IO_BASE), (buf), (nl))
#define outsl_ns(port, buf, nl)	_outsl_ns((unsigned long *)((port)+_IO_BASE), (buf), (nl))

extern void _insw_ns(volatile unsigned short *port, void *buf, int ns);
extern void _outsw_ns(volatile unsigned short *port, const void *buf, int ns);
extern void _insl_ns(volatile unsigned long *port, void *buf, int nl);
extern void _outsl_ns(volatile unsigned long *port, const void *buf, int nl);

#define memset_io(a,b,c)	memset((a),(b),(c))
#define memcpy_fromio(a,b,c)	memcpy((a),(b),(c))
#define memcpy_toio(a,b,c)	memcpy((a),(b),(c))

#ifdef __KERNEL__
/*
 * Map in an area of physical address space, for accessing
 * I/O devices etc.
 */
extern void *__ioremap(unsigned long address, unsigned long size,
		       unsigned long flags);
extern void *ioremap(unsigned long address, unsigned long size);
#define ioremap_nocache(addr, size)	ioremap((addr), (size))
extern void iounmap(void *addr);
extern unsigned long iopa(unsigned long addr);

extern unsigned long mm_ptov(unsigned long addr) __attribute__ ((const));

/*
 * The PCI bus is inherently Little-Endian.  The PowerPC is being
 * run Big-Endian.  Thus all values which cross the [PCI] barrier
 * must be endian-adjusted.  Also, the local DRAM has a different
 * address from the PCI point of view, thus buffer addresses also
 * have to be modified [mapped] appropriately.
 */
extern inline unsigned long virt_to_bus(volatile void * address)
{
	return iopa ((unsigned long) address);
}

extern inline void * bus_to_virt(unsigned long address)
{
	return (void*) mm_ptov (address);
}

/*
 * Change virtual addresses to physical addresses and vv, for
 * addresses in the area where the kernel has the RAM mapped.
 */
extern inline unsigned long virt_to_phys(volatile void * address)
{
	return iopa ((unsigned long) address);
}

extern inline void * phys_to_virt(unsigned long address)
{
	return (void*) mm_ptov (address);
}

#endif /* __KERNEL__ */


/* Enforce in-order execution of data I/O. 
 * No barriers needed for i370.
 */
#define iobarrier_rw() 
#define iobarrier_r() 
#define iobarrier_w() 

/*
 * 8, 16 and 32 bit, big and little endian I/O operations, with barrier.
 */
extern inline int in_8(volatile unsigned char *addr)
{
	int ret;

	/* this will not do a single-byte memory access but I don't think
	 * that matters ... ?! */
	__asm__ __volatile__("la %0,0; ic %0,%1" : "=r" (ret) : "m" (*addr));
	return ret;
}

extern inline void out_8(volatile unsigned char *addr, int val)
{
	__asm__ __volatile__("stc %1,%0" : "=m" (*addr) : "r" (val));
}

extern inline int in_le16(volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__("lhbrx %0,0,%1; eieio" : "=r" (ret) :
			      "r" (addr), "m" (*addr));
	return ret;
}

extern inline int in_be16(volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__("lhz%U1%X1 %0,%1; eieio" : "=r" (ret) : "m" (*addr));
	return ret;
}

extern inline void out_le16(volatile unsigned short *addr, int val)
{
	__asm__ __volatile__("sthbrx %1,0,%2; eieio" : "=m" (*addr) :
			      "r" (val), "r" (addr));
}

extern inline void out_be16(volatile unsigned short *addr, int val)
{
	__asm__ __volatile__("sth%U0%X0 %1,%0; eieio" : "=m" (*addr) : "r" (val));
}

extern inline unsigned in_le32(volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwbrx %0,0,%1; eieio" : "=r" (ret) :
			     "r" (addr), "m" (*addr));
	return ret;
}

extern inline unsigned in_be32(volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwz%U1%X1 %0,%1; eieio" : "=r" (ret) : "m" (*addr));
	return ret;
}

extern inline void out_le32(volatile unsigned *addr, int val)
{
	__asm__ __volatile__("stwbrx %1,0,%2; eieio" : "=m" (*addr) :
			     "r" (val), "r" (addr));
}

extern inline void out_be32(volatile unsigned *addr, int val)
{
	__asm__ __volatile__("stw%U0%X0 %1,%0; eieio" : "=m" (*addr) : "r" (val));
}

#ifdef __KERNEL__
static inline int check_signature(unsigned long io_addr,
	const unsigned char *signature, int length)
{
	int retval = 0;
	do {
		if (readb(io_addr) != *signature)
			goto out;
		io_addr++;
		signature++;
		length--;
	} while (length);
	retval = 1;
out:
	return retval;
}
#endif /* __KERNEL__ */

#endif
