#include <linux/config.h>

/* 
 * XXX almost everything here is wrong ...
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

#define flush_tlb_all i370_flush_tlb_all
#define flush_tlb_mm i370_flush_tlb_mm
#define flush_tlb_page i370_flush_tlb_page
#define flush_tlb_range i370_flush_tlb_range

/*
 * No cache flushing is required 
 */
#define flush_cache_all()		do { } while (0)
#define flush_cache_mm(mm)		do { } while (0)
#define flush_cache_range(mm, a, b)	do { } while (0)
#define flush_cache_page(vma, p)	do { } while (0)

extern void flush_icache_range(unsigned long, unsigned long);
extern void flush_page_to_ram(unsigned long);

extern unsigned long va_to_phys(unsigned long address);
extern pte_t *va_to_pte(struct task_struct *tsk, unsigned long address);
#endif /* __ASSEMBLY__ */

/* XXX this is all wrong */
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
#define PTRS_PER_PTE	1024
#define PTRS_PER_PMD	1
#define PTRS_PER_PGD	1024
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
#define _PAGE_INVALID	0x400	/* pte contains a translation */
#define _PAGE_RW	0x200	/* write access allowed */
#define _PAGE_DIRTY	0x002	/* storage key C: page changed */
#define _PAGE_ACCESSED	0x004	/* storage key R: page referenced */
#define _PAGE_USER	0x001	/* software: user-mode */
#define _PAGE_HWWRITE	0x008	/* software: _PAGE_RW & _PAGE_DIRTY */
#define _PAGE_SHARED	0x010

#define _PAGE_CHG_MASK	(PAGE_MASK | _PAGE_ACCESSED | _PAGE_DIRTY)

#define _PAGE_BASE	 _PAGE_ACCESSED
#define _PAGE_WRENABLE	_PAGE_RW | _PAGE_DIRTY | _PAGE_HWWRITE

#define PAGE_NONE	__pgprot(_PAGE_INVALID | _PAGE_ACCESSED)

#define PAGE_SHARED	__pgprot(_PAGE_BASE | _PAGE_RW | _PAGE_USER | \
				 _PAGE_SHARED)
#define PAGE_COPY	__pgprot(_PAGE_BASE | _PAGE_USER)
#define PAGE_READONLY	__pgprot(_PAGE_BASE | _PAGE_USER)
#define PAGE_KERNEL	__pgprot(_PAGE_BASE | _PAGE_WRENABLE | _PAGE_SHARED)
#define PAGE_KERNEL_CI	__pgprot(_PAGE_BASE | _PAGE_WRENABLE | _PAGE_SHARED )

/*
 * The PowerPC can only do execute protection on a segment (256MB) basis,
 * not on a page basis.  So we consider execute permission the same as read.
 * Also, write permissions imply read permissions.
 * This is the closest we can get..
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
/* 64-bit machines, beware!  SRB. XXX hack alert */
#define SIZEOF_PTR_LOG2	2

/* to set the page-dir */
/* tsk is a task_struct and pgdir is a pte_t */
#define SET_PAGE_DIR(tsk,pgdir)  { 				\
	(tsk)->tss.pg_tables = (unsigned long *)(pgdir);	\
	(tsk)->tss.regs->cr1.raw = 0;				\
	(tsk)->tss.regs->cr1.bits.psto = ((unsigned long) pgdir) >>12;		\
	(tsk)->tss.regs->cr1.bits.pstl = 63;			\
}
     
#ifndef __ASSEMBLY__
extern inline int pte_none(pte_t pte)		{ return !pte_val(pte); }
extern inline int pte_present(pte_t pte)	{ return !(pte_val(pte) & _PAGE_INVALID); }
extern inline void pte_clear(pte_t *ptep)	{ pte_val(*ptep) = 0; }

extern inline int pmd_none(pmd_t pmd)		{ return !pmd_val(pmd); }
extern inline int pmd_bad(pmd_t pmd)		{ return (pmd_val(pmd) & ~PAGE_MASK) != 0; }
extern inline int pmd_present(pmd_t pmd)	{ return (pmd_val(pmd) & PAGE_MASK) != 0; }
extern inline void pmd_clear(pmd_t * pmdp)	{ pmd_val(*pmdp) = 0; }


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
extern inline int pte_write(pte_t pte)		{ return pte_val(pte) & _PAGE_RW; }
extern inline int pte_exec(pte_t pte)		{ return pte_val(pte) & _PAGE_USER; }
extern inline int pte_dirty(pte_t pte)		{ return pte_val(pte) & _PAGE_DIRTY; }
extern inline int pte_young(pte_t pte)		{ return pte_val(pte) & _PAGE_ACCESSED; }

extern inline void pte_uncache(pte_t pte)       { }
extern inline void pte_cache(pte_t pte)         { }

extern inline pte_t pte_rdprotect(pte_t pte) {
	pte_val(pte) &= ~_PAGE_USER; return pte; }
extern inline pte_t pte_exprotect(pte_t pte) {
	pte_val(pte) &= ~_PAGE_USER; return pte; }
extern inline pte_t pte_wrprotect(pte_t pte) {
	pte_val(pte) &= ~(_PAGE_RW | _PAGE_HWWRITE); return pte; }
extern inline pte_t pte_mkclean(pte_t pte) {
	pte_val(pte) &= ~(_PAGE_DIRTY | _PAGE_HWWRITE); return pte; }
extern inline pte_t pte_mkold(pte_t pte) {
	pte_val(pte) &= ~_PAGE_ACCESSED; return pte; }

extern inline pte_t pte_mkread(pte_t pte) {
	pte_val(pte) |= _PAGE_USER; return pte; }
extern inline pte_t pte_mkexec(pte_t pte) {
	pte_val(pte) |= _PAGE_USER; return pte; }
extern inline pte_t pte_mkwrite(pte_t pte)
{
	pte_val(pte) |= _PAGE_RW;
	if (pte_val(pte) & _PAGE_DIRTY)
		pte_val(pte) |= _PAGE_HWWRITE;
	return pte;
}
extern inline pte_t pte_mkdirty(pte_t pte)
{
	pte_val(pte) |= _PAGE_DIRTY;
	if (pte_val(pte) & _PAGE_RW)
		pte_val(pte) |= _PAGE_HWWRITE;
	return pte;
}
extern inline pte_t pte_mkyoung(pte_t pte) {
	pte_val(pte) |= _PAGE_ACCESSED; return pte; }

/* Certain architectures need to do special things when pte's
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
#define set_pte(pteptr, pteval)	((*(pteptr)) = (pteval))

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */

static inline pte_t mk_pte_phys(unsigned long page, pgprot_t pgprot)
{ pte_t pte; 
printk ("mk_pte_phys %lx %lx\n",page,pgprot_val(pgprot));
pte_val(pte) = (page) | pgprot_val(pgprot); return pte; }

extern inline pte_t mk_pte(unsigned long page, pgprot_t pgprot)
{ pte_t pte; 
printk ("mk_pte %lx %lx\n",page, pgprot_val(pgprot));
pte_val(pte) = __pa(page) | pgprot_val(pgprot); return pte; }

extern inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{ pte_val(pte) = (pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot); 
printk ("pte_modify %lx %lx\n", pte_val(pte),pgprot_val(newprot));
return pte; }

extern inline unsigned long pte_page(pte_t pte)
{ return (unsigned long) __va(pte_val(pte) & PAGE_MASK); }

extern inline unsigned long pmd_page(pmd_t pmd)
{ return pmd_val(pmd); }


/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

/* to find an entry in a page-table-directory */
extern inline pgd_t * pgd_offset(struct mm_struct * mm, unsigned long address)
{
	return mm->pgd + (address >> PGDIR_SHIFT);
}

/* Find an entry in the second-level page table.. */
extern inline pmd_t * pmd_offset(pgd_t * dir, unsigned long address)
{
	return (pmd_t *) dir;
}

/* Find an entry in the third-level page table.. */ 
extern inline pte_t * pte_offset(pmd_t * dir, unsigned long address)
{
	return (pte_t *) pmd_page(*dir) + ((address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1));
}

extern __inline__ pgd_t *i370_pgd_alloc(void)
{
	pgd_t *ret, *init;

	ret = (pgd_t *)__get_free_page(GFP_KERNEL);
	if (ret) {
		init = pgd_offset(&init_mm, 0);
		memset (ret, 0, USER_PTRS_PER_PGD * sizeof(pgd_t));
		memcpy (ret + USER_PTRS_PER_PGD, init + USER_PTRS_PER_PGD,
			(PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
	}
	return ret;
}

extern __inline__ void i370_pgd_free(pgd_t *pgd)
{
	free_page((unsigned long)pgd);
}


extern __inline__ void i370_pte_free(pte_t *pte)
{
	free_page((unsigned long)pte);
}

extern void __bad_pte(pmd_t *pmd);

#define pte_free_kernel(pte)    i370_pte_free(pte)
#define pte_free(pte)           i370_pte_free(pte)
#define pgd_free(pgd)           i370_pgd_free(pgd)
#define pgd_alloc()             i370_pgd_alloc()

extern pte_t *get_pte_slow(pmd_t *pmd, unsigned long address_preadjusted);

extern inline pte_t * pte_alloc(pmd_t * pmd, unsigned long address)
{
	address = (address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1);
	if (pmd_none(*pmd)) {
		return get_pte_slow(pmd, address);
	}
	if (pmd_bad(*pmd)) {
		__bad_pte(pmd);
		return NULL;
	}
	return (pte_t *) pmd_page(*pmd) + address;
}

/*
 * allocating and freeing a pmd is trivial: the 1-entry pmd is
 * inside the pgd, so has no extra memory associated with it.
 */
extern inline void pmd_free(pmd_t * pmd)
{
}

extern inline pmd_t * pmd_alloc(pgd_t * pgd, unsigned long address)
{
	return (pmd_t *) pgd;
}

#define pmd_free_kernel		pmd_free
#define pmd_alloc_kernel	pmd_alloc
#define pte_alloc_kernel	pte_alloc

extern int do_check_pgt_cache(int, int);

extern inline void set_pgdir(unsigned long address, pgd_t entry)
{
	struct task_struct * p;
#ifdef __SMP__
	int i;
#endif	
        
	read_lock(&tasklist_lock);
	for_each_task(p) {
		if (!p->mm)
			continue;
		*pgd_offset(p->mm,address) = entry;
	}
	read_unlock(&tasklist_lock);
}

extern pgd_t swapper_pg_dir[1024];

extern __inline__ pte_t *find_pte(struct mm_struct *mm,unsigned long va)
{
	pgd_t *dir;
	pmd_t *pmd;
	pte_t *pte;

	va &= PAGE_MASK;
	
	dir = pgd_offset( mm, va );
	if (dir)
	{
		pmd = pmd_offset(dir, va & PAGE_MASK);
		if (pmd && pmd_present(*pmd))
		{
			pte = pte_offset(pmd, va);
			if (pte && pte_present(*pte))
			{			
				pte_uncache(*pte);
				flush_tlb_page(find_vma(mm,va),va);
			}
		}
	}
	return pte;
}

/*
 * Page tables may have changed.  We don't need to do anything here
 * as entries are faulted into the hash table by the low-level
 * data/instruction access exception handlers.
 */
#define update_mmu_cache(vma, addr, pte)	do { } while (0)


#define SWP_TYPE(entry) (((entry) >> 1) & 0x7f)
#define SWP_OFFSET(entry) ((entry) >> 8)
#define SWP_ENTRY(type,offset) (((type) << 1) | ((offset) << 8))

#define module_map      vmalloc
#define module_unmap    vfree

/* For virtual address to physical address conversion */
extern void cache_clear(__u32 addr, int length);
extern void cache_push(__u32 addr, int length);
extern int mm_end_of_chunk (unsigned long addr, int len);
extern unsigned long iopa(unsigned long addr);
extern unsigned long mm_ptov(unsigned long addr) __attribute__ ((const));

/*
 * Map some physical address range into the kernel address space.
 */
extern unsigned long kernel_map(unsigned long paddr, unsigned long size,
				int nocacheflag, unsigned long *memavailp );

/*
 * Set cache mode of (kernel space) address range. 
 */
extern void kernel_set_cachemode (unsigned long address, unsigned long size,
                                 unsigned int cmode);

/* Needs to be defined here and not in linux/mm.h, as it is arch dependent */
#define PageSkip(page)		(0)
#define kern_addr_valid(addr)	(1)

#endif __ASSEMBLY__
#endif /* _I370_PGTABLE_H */
