/* $Id: ide.h,v 1.1.1.1 1999/02/08 06:19:10 linas Exp $
 * ide.h: Ultra/PCI specific IDE glue.
 *
 * Copyright (C) 1997  David S. Miller (davem@caip.rutgers.edu)
 * Copyright (C) 1998  Eddie C. Dost   (ecd@skynet.be)
 */

#ifndef _SPARC64_IDE_H
#define _SPARC64_IDE_H

#ifdef __KERNEL__

#include <asm/pgtable.h>

typedef unsigned long ide_ioreg_t;

#undef  MAX_HWIFS
#define MAX_HWIFS	2

#define	ide__sti()	__sti()

static __inline__ int ide_default_irq(ide_ioreg_t base)
{
	return 0;
}

static __inline__ ide_ioreg_t ide_default_io_base(int index)
{
	return 0;
}

static __inline__ void ide_init_hwif_ports(ide_ioreg_t *p, ide_ioreg_t base, int *irq)
{
	int i;

	/* These are simply offsets from base. */
	for (i = 0; i < 8; i++)
		*p++ = base++;
	/* PCI code needs to figure out these. */
	for ( ; i < 10; i++)
		*p++ = 0;
	/* PCI code needs to figure out this. */
	if (irq != NULL)
		*irq = 0;
}

typedef union {
	unsigned int		all	: 8;	/* all of the bits together */
	struct {
		unsigned int	bit7	: 1;
		unsigned int	lba	: 1;
		unsigned int	bit5	: 1;
		unsigned int	unit	: 1;
		unsigned int	head	: 4;
	} b;
} select_t;

static __inline__ int ide_request_irq(unsigned int irq,
				      void (*handler)(int, void *, struct pt_regs *),
				      unsigned long flags, const char *name, void *devid)
{
	return request_irq(irq, handler, SA_SHIRQ, name, devid);
}

static __inline__ void ide_free_irq(unsigned int irq, void *dev_id)
{
	free_irq(irq, dev_id);
}

static __inline__ int ide_check_region(ide_ioreg_t base, unsigned int size)
{
	return check_region(base, size);
}

static __inline__ void ide_request_region(ide_ioreg_t base, unsigned int size,
					  const char *name)
{
	request_region(base, size, name);
}

static __inline__ void ide_release_region(ide_ioreg_t base, unsigned int size)
{
	release_region(base, size);
}

#undef  SUPPORT_SLOW_DATA_PORTS
#define SUPPORT_SLOW_DATA_PORTS 0

#undef  SUPPORT_VLB_SYNC
#define SUPPORT_VLB_SYNC 0

#undef  HD_DATA
#define HD_DATA ((ide_ioreg_t)0)

static __inline__ int ide_ack_intr(ide_ioreg_t status_port, ide_ioreg_t irq_port)
{
	return 1;
}

/* From m68k code... */

#ifdef insl
#undef insl
#endif
#ifdef outsl
#undef outsl
#endif
#ifdef insw
#undef insw
#endif
#ifdef outsw
#undef outsw
#endif

#define insl(data_reg, buffer, wcount) insw(data_reg, buffer, (wcount)<<1)
#define outsl(data_reg, buffer, wcount) outsw(data_reg, buffer, (wcount)<<1)

#define insw(port, buf, nr) ide_insw((port), (buf), (nr))
#define outsw(port, buf, nr) ide_outsw((port), (buf), (nr))

static __inline__ void ide_insw(unsigned long port,
				void *dst,
				unsigned long count)
{
	volatile unsigned short *data_port;
	unsigned long end = (unsigned long)dst + count;
	u16 *ps = dst;
	u32 *pi;

	data_port = (volatile unsigned short *)port;

	if(((u64)ps) & 0x2) {
		*ps++ = *data_port;
		count--;
	}
	pi = (u32 *)ps;
	while(count >= 2) {
		u32 w;

		w  = (*data_port) << 16;
		w |= (*data_port);
		*pi++ = w;
		count -= 2;
	}
	ps = (u16 *)pi;
	if(count)
		*ps++ = *data_port;

	__flush_dcache_range((unsigned long)dst, end);
}

static __inline__ void ide_outsw(unsigned long port,
				 const void *src,
				 unsigned long count)
{
	volatile unsigned short *data_port;
	unsigned long end = (unsigned long)src + count;
	const u16 *ps = src;
	const u32 *pi;

	data_port = (volatile unsigned short *)port;

	if(((u64)src) & 0x2) {
		*data_port = *ps++;
		count--;
	}
	pi = (const u32 *)ps;
	while(count >= 2) {
		u32 w;

		w = *pi++;
		*data_port = (w >> 16);
		*data_port = w;
		count -= 2;
	}
	ps = (const u16 *)pi;
	if(count)
		*data_port = *ps;

	__flush_dcache_range((unsigned long)src, end);
}

#define T_CHAR          (0x0000)        /* char:  don't touch  */
#define T_SHORT         (0x4000)        /* short: 12 -> 21     */
#define T_INT           (0x8000)        /* int:   1234 -> 4321 */
#define T_TEXT          (0xc000)        /* text:  12 -> 21     */

#define T_MASK_TYPE     (0xc000)
#define T_MASK_COUNT    (0x3fff)

#define D_CHAR(cnt)     (T_CHAR  | (cnt))
#define D_SHORT(cnt)    (T_SHORT | (cnt))
#define D_INT(cnt)      (T_INT   | (cnt))
#define D_TEXT(cnt)     (T_TEXT  | (cnt))

static u_short driveid_types[] = {
	D_SHORT(10),	/* config - vendor2 */
	D_TEXT(20),	/* serial_no */
	D_SHORT(3),	/* buf_type - ecc_bytes */
	D_TEXT(48),	/* fw_rev - model */
	D_CHAR(2),	/* max_multsect - vendor3 */
	D_SHORT(1),	/* dword_io */
	D_CHAR(2),	/* vendor4 - capability */
	D_SHORT(1),	/* reserved50 */
	D_CHAR(4),	/* vendor5 - tDMA */
	D_SHORT(4),	/* field_valid - cur_sectors */
	D_INT(1),	/* cur_capacity */
	D_CHAR(2),	/* multsect - multsect_valid */
	D_INT(1),	/* lba_capacity */
	D_SHORT(194)	/* dma_1word - reservedyy */
};

#define num_driveid_types       (sizeof(driveid_types)/sizeof(*driveid_types))

static __inline__ void ide_fix_driveid(struct hd_driveid *id)
{
	u_char *p = (u_char *)id;
	int i, j, cnt;
	u_char t;

	for (i = 0; i < num_driveid_types; i++) {
		cnt = driveid_types[i] & T_MASK_COUNT;
		switch (driveid_types[i] & T_MASK_TYPE) {
		case T_CHAR:
			p += cnt;
			break;
		case T_SHORT:
			for (j = 0; j < cnt; j++) {
				t = p[0];
				p[0] = p[1];
				p[1] = t;
				p += 2;
			}
			break;
		case T_INT:
			for (j = 0; j < cnt; j++) {
				t = p[0];
				p[0] = p[3];
				p[3] = t;
				t = p[1];
				p[1] = p[2];
				p[2] = t;
				p += 4;
			}
			break;
		case T_TEXT:
			for (j = 0; j < cnt; j += 2) {
				t = p[0];
				p[0] = p[1];
				p[1] = t;
				p += 2;
			}
			break;
		};
	}
}

static __inline__ void ide_release_lock (int *ide_lock)
{
}

static __inline__ void ide_get_lock (int *ide_lock, void (*handler)(int, void *, struct pt_regs *), void *data)
{
}

#endif /* __KERNEL__ */

#endif /* _SPARC64_IDE_H */
