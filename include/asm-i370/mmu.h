/*
 * i370 memory management structures
 */

#ifndef _I370_MMU_H_
#define _I370_MMU_H_

#ifndef __ASSEMBLY__

/* Hardware Page Table Entry */
typedef struct _i370_pte_s {
	unsigned long za:1;	/* Must be zero */
	unsigned long pfra:19;	/* Page Frame Real Adress */
	unsigned long zb:1;	/* Must be zero */
	unsigned long iv:1;	/* Page Invalid bit */
	unsigned long pp:1;	/* Page Protection bit */
	unsigned long zc:9;	/* Must be zero */
} i370_pte_t; 

/* Segment Table Entry */
typedef struct _i370_ste_s {
	unsigned long za:1;	/* Must be zero */
	unsigned long pto:25;	/* Page Table Origin */
	unsigned long iv:1;	/* Segment Invalid bit */
	unsigned long c:1;	/* Common-Segment bit */
	unsigned long ptl:4;	/* Page Table Length */
} i370_ste_t;


/* XXX everything below is all hogwash and wrong */
/*
 * Simulated two-level MMU.  This structure is used by the kernel
 * to keep track of MMU mappings and is used to update/maintain
 * the hardware table. 
 *
 * The simulated structures mimic the hardware available on other
 * platforms, notably the 80x86 and 680x0.
 */

typedef struct _pte {
   	unsigned long page_num:20;
   	unsigned long flags:12;		/* Page flags (some unused bits) */
} pte;

#define PD_SHIFT (8+12)		/* Page directory */
#define PD_MASK  0x02FF
#define PT_SHIFT (12)			/* Page Table */
#define PT_MASK  0x02FF
#define PG_SHIFT (12)			/* Page Entry */
 
#endif /* __ASSEMBLY__ */

#endif /* _I370_MMU_H_ */
