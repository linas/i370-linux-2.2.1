#include <linux/config.h>
/* 
 * Page Table mamangement stuff for the ESA/390.  
 * XXX under constructions, some things here might be very wrong ...
 */

#ifndef _I370_PGTABLE_H
#define _I370_PGTABLE_H

#ifndef __ASSEMBLY__
#include <linux/mm.h>
#include <asm/processor.h>		/* For TASK_SIZE */
#include <asm/mmu.h>
#include <asm/page.h>

extern void i370_flush_tlb_all(void);
extern void i370_flush_tlb_mm(struct mm_struct *mm);
extern void i370_flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr);
extern void i370_flush_tlb_range(struct mm_struct *mm, unsigned long start,
			    unsigned long end);

#define flush_tlb_all   i370_flush_tlb_all
#define flush_tlb_mm    i370_flush_tlb_mm
#define flush_tlb_page  i370_flush_tlb_page
#define flush_tlb_range i370_flush_tlb_range

/*
 * No cache flushing is required on i370 architecture
 */
#define flush_cache_all()		do { } while (0)
#define flush_cache_mm(mm)		do { } while (0)
#define flush_cache_range(mm, a, b)	do { } while (0)
#define flush_cache_page(vma, p)	do { } while (0)

#define flush_icache_range(a,b)		do { } while (0)
#define flush_page_to_ram(a)		do { } while (0)

extern unsigned long va_to_phys(unsigned long address);
extern pte_t *va_to_pte(struct task_struct *tsk, unsigned long address);
#endif /* __ASSEMBLY__ */

/*
 * The ESA/390 MMU uses a page table containing PTEs, together with
 * a segment table containing pointers to page tables to define
 * the virtual to physical address mapping. We map the linux pgd to
 * the segment table.
 */

/* PMD_SHIFT determines the size of the area mapped by the second-level
 * page tables.  For the i370, it is a 1MB segment 
 */
#define PMD_SHIFT	20
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))

/* PGDIR_SHIFT determines what a third-level page table entry can map */
#define PGDIR_SHIFT	20
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

/*
 * entries per page directory level: our page-table tree is two-level, so
 * we don't really have any PMD directory.
 */
#define PTRS_PER_PTE	256
#define PTRS_PER_PMD	1
#define PTRS_PER_PGD	2048

/* Because each i370 page table has only 256 entries, we can in fact
 * fit four page tables onto one page. (256*4 *4bytes per entry = 4096 bytes) */
#define PT_PER_PAGE	4

/* segment table entry page table origin mask */
#define SEG_PTO_MASK	0x7fffffc0

/* page frame real address mask */
#define PFRA_MASK	0x7ffff000

/* USER_PTRS_PER_PGD should equal 2048 to address 2GB */
#define USER_PTRS_PER_PGD	(TASK_SIZE / PGDIR_SIZE)

/* VMALLOC_OFFSET is an (arbitrary) offset to the start of the vmalloc 
 * kernel VM area: the current 1GB value means that all kernel vm addresses
 * will have a "1" in bit position 30.  
 * We do this to catch out-of-bounds real memory accesses.
 * It also means that we have imposed a practical limit of max of 1GB 
 * a real memory.  This limit can be raised, although it will decrease 
 * the amount of virtual memory available to the kernel.
 * 
 * The vmalloc() routines leaves a hole of 4kB between each vmalloced
 * area for the same reason. ;)
 *
 */
#define VMALLOC_OFFSET	(0x40000000) /* 1GB */
#define VMALLOC_START ((VMALLOC_OFFSET & ~(VMALLOC_OFFSET-1)))
#define VMALLOC_VMADDR(x) ((unsigned long)(x))
#define VMALLOC_END	0x7fffe000

/*
 * Bits in a linux-style PTE.  Two bits match the hardware
 * PTE, the other bits are software-only thingies that inhabit 
 * low byte of the pte and hopefull won't hurt anybody.
 */
#define _PAGE_INVALID	0x400	/* hardware pte: I bit is set */
#define _PAGE_RO	0x200	/* hardware pte: P bit (write protect) */

/* hardware segment table entry */
#define _SEG_INVALID    0x20	/* segment invalid bit */
#define _SEG_COMMON     0x10	/* segment common bit */

#define _PAGE_DIRTY	0x002	/* storage key C: page changed */
#define _PAGE_ACCESSED	0x004	/* storage key R: page referenced */

#define _PAGE_USER	0x001	/* software: user-mode */
#define _PAGE_SHARED	0x010

#define _PAGE_CHG_MASK	(PAGE_MASK | _PAGE_ACCESSED | _PAGE_DIRTY)

#define PAGE_NONE	__pgprot( _PAGE_INVALID)

#define PAGE_SHARED	__pgprot( _PAGE_USER | _PAGE_SHARED)
#define PAGE_COPY	__pgprot( _PAGE_USER)
#define PAGE_READONLY	__pgprot( _PAGE_USER | _PAGE_RO)
#define PAGE_KERNEL	__pgprot( _PAGE_SHARED)
#define PAGE_KERNEL_CI	__pgprot( _PAGE_SHARED )

/*
 * We consider execute permission the same as read.
 * Also, write permissions imply read permissions.
 * If we go fancy, maybe we can do better but this will hold for now.
 * P macros imply copy-on-write, S macros for shared access
 */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY
#define __S101	PAGE_READONLY
#define __S110	PAGE_SHARED
#define __S111	PAGE_SHARED

/*
 * BAD_PAGETABLE is used when we need a bogus page-table, while
 * BAD_PAGE is used for a bogus page.
 *
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
#ifndef __ASSEMBLY__
extern pte_t __bad_page(void);
extern pte_t * __bad_pagetable(void);

extern unsigned long empty_zero_page[1024];
#endif __ASSEMBLY__
#define BAD_PAGETABLE	__bad_pagetable()
#define BAD_PAGE	__bad_page()
#define ZERO_PAGE	((unsigned long) empty_zero_page)

/* number of bits that fit into a memory pointer */
#define BITS_PER_PTR	(8*sizeof(unsigned long))

/* to align the pointer to a pointer address */
#define PTR_MASK	(~(sizeof(void*)-1))

/* sizeof(void*) == 1<<SIZEOF_PTR_LOG2 */
#define SIZEOF_PTR_LOG2	2

/* to set the page-dir */
/* tsk is a task_struct and pgdir is a pgd_t* */
#define SET_PAGE_DIR(tsk,pgdir)  { 				\
	(tsk)->tss.pg_tables = (unsigned long *)(pgdir);	\
	(tsk)->tss.regs->cr1.raw = 0;				\
	(tsk)->tss.regs->cr1.bits.psto = ((unsigned long) pgdir) >>12;		\
	(tsk)->tss.regs->cr1.bits.pstl = 127;			\
	_lctl_r1 ((tsk)->tss.regs->cr1.raw);			\
}
     
#ifndef __ASSEMBLY__

/* Unused pte's must have the invalid bit set ...  */
extern inline int 
	pte_none(pte_t pte)	{ return (pte_val(pte) & _PAGE_INVALID); }
extern inline int 
	pte_present(pte_t pte)	{ return !((pte_val(pte) & _PAGE_INVALID)); }
extern inline void 
	pte_clear(pte_t *ptep)	{ pte_val(*ptep) = _PAGE_INVALID | _PAGE_RO; }


/* unused segment table entries must have the invalid bit set.
 * XXX what's pmd_bad supposed to mean ????
 */
extern inline int 
	pmd_none(pmd_t pmd)	{ return pmd_val(pmd) & _SEG_INVALID; }
extern inline int 
	pmd_bad(pmd_t pmd)	{ return 0; }
extern inline int 
	pmd_present(pmd_t pmd)	{ return (pmd_val(pmd) & _SEG_INVALID) == 0; }
extern inline void 
	pmd_clear(pmd_t * pmdp)	{ pmd_val(*pmdp) = _SEG_INVALID; }


/*
 * The "pgd_xxx()" functions here are trivial for a folded two-level
 * setup: the pgd is never bad, and a pmd always exists (as it's folded
 * into the pgd entry)
 */
extern inline int pgd_none(pgd_t pgd)		{ return 0; }
extern inline int pgd_bad(pgd_t pgd)		{ return 0; }
extern inline int pgd_present(pgd_t pgd)	{ return 1; }
extern inline void pgd_clear(pgd_t * pgdp)	{ }

/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */
extern inline int pte_read(pte_t pte)		{ return pte_val(pte) & _PAGE_USER; }
extern inline int pte_write(pte_t pte)		{ return !(pte_val(pte) & _PAGE_RO); }
extern inline int pte_exec(pte_t pte)		{ return pte_val(pte) & _PAGE_USER; }

/* XXX we should get ref & change bits out of storage key */
extern inline int pte_dirty(pte_t pte)		{ return pte_val(pte) & _PAGE_DIRTY; }
extern inline int pte_young(pte_t pte)		{ return !(pte_val(pte) & _PAGE_ACCESSED); }

extern inline void pte_uncache(pte_t pte)       { }
extern inline void pte_cache(pte_t pte)         { }

extern inline pte_t 
	pte_rdprotect(pte_t pte) { pte_val(pte) &= ~_PAGE_USER; return pte; }
extern inline pte_t 
	pte_mkread(pte_t pte) 	{ pte_val(pte) |= _PAGE_USER; return pte; }

extern inline pte_t 
	pte_exprotect(pte_t pte) { pte_val(pte) &= ~_PAGE_USER; return pte; }
extern inline pte_t 
	pte_mkexec(pte_t pte) 	{ pte_val(pte) |= _PAGE_USER; return pte; }

extern inline pte_t 
	pte_wrprotect(pte_t pte) { pte_val(pte) |= _PAGE_RO ; return pte; }
extern inline pte_t 
	pte_mkwrite(pte_t pte) 	{ pte_val(pte) &= ~_PAGE_RO; return pte; }

extern inline pte_t 
	pte_mkclean(pte_t pte) 	{ pte_val(pte) &= ~(_PAGE_DIRTY ); return pte; }
extern inline pte_t 
	pte_mkdirty(pte_t pte) 	{ pte_val(pte) |= _PAGE_DIRTY; return pte; }

extern inline pte_t 
	pte_mkold(pte_t pte)  	{ pte_val(pte) |= _PAGE_ACCESSED; return pte; }
extern inline pte_t 
	pte_mkyoung(pte_t pte) 	{ pte_val(pte) &= ~_PAGE_ACCESSED; return pte; }



/* Certain architectures need to do special things when pte's
 * within a page table are directly modified.  Thus, the following
 * hook is made available. 
 * For the ESA/390, I think we're supposed to set/reset the
 * reference & change bits in the key storage at this time ...
 * XXX right ?? implement this ... 
 */
#define set_pte(pteptr, pteval)	((*(pteptr)) = (pteval))

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */

static inline pte_t mk_pte_phys(unsigned long page, pgprot_t pgprot)
{ pte_t pte; 
printk ("mk_pte_phys %lx %lx\n",page,pgprot_val(pgprot));
pte_val(pte) = (page & PFRA_MASK) | pgprot_val(pgprot); return pte; }

extern inline pte_t mk_pte(unsigned long page, pgprot_t pgprot)
{ pte_t pte; 
printk ("mk_pte %lx %lx\n",page, pgprot_val(pgprot));
pte_val(pte) = (page & PFRA_MASK) | pgprot_val(pgprot); return pte; }

extern inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{ pte_val(pte) = (pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot); 
printk ("pte_modify %lx %lx\n", pte_val(pte),pgprot_val(newprot));
return pte; }

/* pte_page returns only the addrss part of the entry; mask out misc bits */
extern inline unsigned long pte_page(pte_t pte)
{ return (unsigned long) (pte_val(pte) & PFRA_MASK); }

/* pmd_page returns only the address part of the entry; mask out misc bits */
extern inline unsigned long pmd_page(pmd_t pmd)
{ return pmd_val(pmd) & SEG_PTO_MASK; }


/* to find an entry in a kernel segment-table-directory */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

/* to find an entry in a segment-table-directory */
extern inline pgd_t * pgd_offset(struct mm_struct * mm, unsigned long address)
{
	/* addend should be between 0 and 2048 */
	return mm->pgd + ((address & ADDR_MASK) >> PGDIR_SHIFT);
}

/* Find an entry in the second-level page table.. */
extern inline pmd_t * pmd_offset(pgd_t * dir, unsigned long address)
{
	return (pmd_t *) dir;
}

/* Find an entry in the third-level page table.. */ 
extern inline pte_t * pte_offset(pmd_t * dir, unsigned long address)
{
	pte_t * ppp = (pte_t *) pmd_page(*dir);
	/* addend should be between 0 and 256 */
	ppp += (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
	return ppp;
}

extern __inline__ pgd_t *i370_pgd_alloc(void)
{
	pgd_t *ret;

	/* grab two pages, for 2048 entires mapping 2GB */
	ret = (pgd_t *)__get_free_pages(GFP_KERNEL,1);
	if (ret) {
		/* zero out the entire pgd, not just the 'user' part of it */
		/* memset (ret, 0, USER_PTRS_PER_PGD * sizeof(pgd_t)); */
		/* memset won't work, we need to set the invalid bit */
		int i;
		pmd_t *pme = (pmd_t *) ret;
printk ("in pgd_alloc alloced 2 pages at %p\n", ret);
		for (i=0; i<USER_PTRS_PER_PGD; i++) {
			pmd_clear (pme);
			pme ++;
		}
		
	}
	return ret;
}

extern __inline__ void i370_pgd_free(pgd_t *pgd)
{
	/* free two pages of the pgd */
	free_pages((unsigned long)pgd,1);
}


extern __inline__ void i370_pte_free(pte_t *pte)
{
	free_page((unsigned long)pte);
}

extern void __bad_pte(pmd_t *pmd);

#define pte_alloc(pmd,addr)     i370_pte_alloc(pmd,addr)
#define pte_free(pte)           i370_pte_free(pte)

#define pte_alloc_kernel	i370_pte_alloc
#define pte_free_kernel(pte)    i370_pte_free(pte)

#define pmd_free		i370_pmd_free
#define pmd_alloc		i370_pmd_alloc
#define pmd_free_kernel		i370_pmd_free
#define pmd_alloc_kernel	i370_pmd_alloc

#define pgd_alloc()             i370_pgd_alloc()
#define pgd_free(pgd)           i370_pgd_free(pgd)

extern pte_t *i370_get_pte(pmd_t *pmd, unsigned long pt_entry_index);

extern inline pte_t * i370_pte_alloc(pmd_t * pmd, unsigned long address)
{
	unsigned long entry_idx = (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
	if (pmd_none(*pmd)) {
		return i370_get_pte(pmd, entry_idx);
	}
	if (pmd_bad(*pmd)) {
		__bad_pte(pmd);
		return NULL;
	}
	return ((pte_t *)pmd_page(*pmd)) + entry_idx;
}

/*
 * allocating and freeing a pmd is trivial: the 1-entry pmd is
 * inside the pgd, so has no extra memory associated with it.
 */
extern inline void i370_pmd_free(pmd_t * pmd)
{
}

extern inline pmd_t * i370_pmd_alloc(pgd_t * pgd, unsigned long address)
{
	return (pmd_t *) pgd;
}


extern int do_check_pgt_cache(int, int);

/* XXX ??? what is this supposed to do ??? */
/* I think this is used only by vmalloc, and is used to make
 * sure that all kernel processes see the virtual page mapping.
 * right now we don't support vmalloc, for the moment.
 */
extern inline void set_pgdir(unsigned long address, pgd_t entry)
{
	printk ("i370_set_pgdir(): unimplemented\n");
	i370_halt();
#if 0
	struct task_struct * p;
        
	read_lock(&tasklist_lock);
	for_each_task(p) {
		if (!p->mm)
			continue;
		*pgd_offset(p->mm,address) = entry;
	}
	read_unlock(&tasklist_lock);
#endif 
}

/* the pgdir consists of *TWO* pages (address 2GB of storage) */
extern pgd_t swapper_pg_dir[2048];

extern __inline__ pte_t *find_pte(struct mm_struct *mm, unsigned long va)
{
	pgd_t *dir;
	pmd_t *pmd;
	pte_t *pte = NULL;

	va &= ADDR_MASK;
printk ("find_pte for va=0x%lx\n", va);
	
	dir = pgd_offset( mm, va );
	if (dir)
	{
printk ("find_pte got pgd for dir=%p\n", dir);
		pmd = pmd_offset(dir, va);
		if (pmd && pmd_present(*pmd))
		{
			pte = pte_offset(pmd, va);
printk ("find_pte got pmd for va=%lx pte=%p\n", va, pte);
		}
	}
	return pte;
}

/*
 * XXX is this correct ???
 * Page tables may have changed.  We don't need to do anything here
 * as entries are faulted into the hash table by the low-level
 * data/instruction access exception handlers.
 */
#define update_mmu_cache(vma, addr, pte)	do { } while (0)


/* XXX what's this ??? */
#define SWP_TYPE(entry) (((entry) >> 1) & 0x7f)
#define SWP_OFFSET(entry) ((entry) >> 8)
#define SWP_ENTRY(type,offset) (((type) << 1) | ((offset) << 8))

#define module_map      vmalloc
#define module_unmap    vfree

/* Needs to be defined here and not in linux/mm.h, as it is arch dependent */
#define PageSkip(page)		(0)

#if 0
/* XXX junk delete me */
#define kern_addr_valid(addr)	(1)
#endif

#endif __ASSEMBLY__
#endif /* _I370_PGTABLE_H */
