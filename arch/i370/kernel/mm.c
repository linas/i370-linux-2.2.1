
/*
 * XXX under construction ... what's in this file works for now,
 * but some of what's in this file is very wrong ...
 */

#include <linux/blk.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/swap.h>

#include <asm/asm.h>
#include <asm/atomic.h>
#include <asm/current.h>
#include <asm/mmu.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>
#include <asm/vmdiag.h>

extern unsigned long free_area_init(unsigned long, unsigned long);
extern __initfunc(void i370_trap_init(int key));

extern char __init_text_begin[], __init_text_end[];
extern char __init_data_begin[], __init_data_end[];
extern char _text[], _etext[];
extern unsigned char *CPUID;


/* mem_init() will put the kernel text pages into a different
 * storage key than the data pages, effectively rendering them read-only.
 * It then changes the key under which the kernel executes.  Remaining
 * area is marked as available for allocation by the Linux kernel.
 */

__initfunc(void mem_init(unsigned long start_mem, unsigned long end_mem))
{
	cr0_t cr0;
	unsigned long addr;
	int lowpages = 0;
	int codepages = 0;
	int datapages = 0;
	int initpages = 0;
	int initrdpages = 0;
	int i;
	pmd_t *pme;

	/* printk ("enter mem init startmem=0x%lx endmem=0x%lx\n", start_mem, end_mem); */
	end_mem &= PAGE_MASK;
	high_memory = (void *) end_mem;
	max_mapnr = MAP_NR(high_memory);

	/* mark usable pages in the mem_map[] */
	start_mem = PAGE_ALIGN(start_mem);
	num_physpages = max_mapnr;      /* RAM is assumed contiguous */

#ifdef CONFIG_BLK_DEV_INITRD
#if defined(CONFIG_VM_GUEST)
	/*-------------------------------------------------------*/
	/* If we are running under VM then we can load the RAM   */
	/* disk from a shared segment (isn't VM wonderful!)      */
	/* VM has CPUID[0] == 0xff and Hercules has != 0x0       */
	/* XXX But this interferes with the later initrd load.   */
	/* So I don''t get it. May want to keep this turned off. */
	/*-------------------------------------------------------*/
	if (CPUID[0] != 0x0)
	{
		VM_Diagnose_Code_64(VMDIAG_LOADNSHR,"RAMDISK ",
				    &initrd_start, &initrd_end);
		printk("RAMDISK NSS loaded\n");
	}
#endif /* CONFIG_VM_GUEST */
#endif /* CONFIG_BLK_DEV_INITRD */

	for (addr = 0x0; addr < end_mem; addr += PAGE_SIZE)
	{
		if (addr < (ulong) start_mem)
		{
			/* mark this page as in use by the kernel */
			set_bit(PG_reserved, &mem_map[MAP_NR(addr)].flags);
			if (addr < (ulong) _text)
			{
				_sske (KTEXT_STORAGE_KEY, addr);
				lowpages++;
			}
			else if (addr < (ulong) _etext)
			{
				_sske (KTEXT_STORAGE_KEY, addr);
				codepages++;
			}
			else if (addr >= (ulong) __init_text_begin
					&& addr < (ulong) __init_text_end)
			{
				_sske (KTEXT_STORAGE_KEY, addr);
				initpages++;
			}
			else if (addr >= (ulong) __init_data_begin
					&& addr < (ulong) __init_data_end)
			{
				_sske (KDATA_STORAGE_KEY, addr);
				initpages++;
			}
			else
			{
				_sske (KDATA_STORAGE_KEY, addr);
				datapages++;
			}
		} else {
			/* The reserved bit was set previously, in page_init
			 * by free_area_init, below.  Clear the bit now. */
			clear_bit(PG_reserved, &mem_map[MAP_NR(addr)].flags);
			atomic_set(&mem_map[MAP_NR(addr)].count, 1);
#ifdef CONFIG_BLK_DEV_INITRD
			if (!initrd_start ||
				addr < (initrd_start & PAGE_MASK) || addr >= initrd_end) {
				free_page(addr);
			 } else {
				initrdpages ++;
			 }
#else  /* CONFIG_BLK_DEV_INITRD */
			 free_page(addr);
#endif /* CONFIG_BLK_DEV_INITRD */

			 _sske (KDATA_STORAGE_KEY, addr);
		}
	}

	/* Mark the first page RW since we need to put interrupt stuff there.
	 * Low address protection is turned on later, below */
	_sske (KDATA_STORAGE_KEY, 0x0);

	/* Take away write privledges from text pages. */
	_spka (KDATA_STORAGE_KEY);

	/* Interrupt vectors can't have the keys to the kingdom either */
	i370_trap_init (KDATA_STORAGE_KEY);

	/* Do allow kernel to access key-9 storage (USER_PSW) --
	   by setting the storage protection override bit in cr0.
	   What about fpoc (fetch protection) ???
	   Maybe we should do this only during copyin/out ??
	   This is, we still want to keep kernel text read-only...
	   Protect addresses in 0-512 from accidental storage.  */
	cr0.raw = _stctl_r0();
	cr0.bits.spoc = 1;  /* storage protection override control */
	cr0.bits.lapc = 1;  /* low address protection control */
	_lctl_r0(cr0.raw);

	printk("Memory: %luk available (%dk code, %dk data, %dk init)\n",
		(unsigned long) nr_free_pages << (PAGE_SHIFT-10),
		codepages << (PAGE_SHIFT-10),
		datapages << (PAGE_SHIFT-10),
		initpages << (PAGE_SHIFT-10));

#ifdef CONFIG_BLK_DEV_INITRD
	printk("Init Ramdisk: %dk  [%08lx,%08lx]\n",
		initrdpages << (PAGE_SHIFT-10),
		initrd_start, initrd_end);
#endif /* CONFIG_BLK_DEV_INITRD */

	/* Initialize kernel address translation tables. */
	pme = (pmd_t *) swapper_pg_dir;
	for (i=0; i<PTRS_PER_PGD; i++) {
		pmd_clear (pme);
		pme ++;
	}
}

__initfunc(void free_initmem(void))
{
	unsigned long num_freed_pages = 0;

	/* This is just a shortened version of the code further up above */
#define FREESEC(START,END,CNT) \
	do { \
		unsigned long a = (unsigned long)(&START); \
		for (; a < (unsigned long)(&END); a += PAGE_SIZE) { \
			_sske (KDATA_STORAGE_KEY, a); \
			clear_bit(PG_reserved, &mem_map[MAP_NR(a)].flags); \
			atomic_set(&mem_map[MAP_NR(a)].count, 1); \
			free_page(a); \
			CNT++; \
		} \
	} while (0)

	FREESEC(__init_text_begin, __init_data_end, num_freed_pages);

	printk ("freed initmem from %p to %p  (%lu pages total)\n",
		__init_text_begin, __init_data_end, num_freed_pages);

	/* XXX hack alert I don't think init_irq belongs here */
	irq_init();
}

void si_meminfo(struct sysinfo *val)
{ }

void show_mem(void)
{ printk (" show mem not implemented\n"); }


/*
 * TLB flushing:
 *
 *  - flush_tlb_all() flushes all processes TLBs
 *  - flush_tlb_mm(mm) flushes the specified mm context TLB's
 *  - flush_tlb_page(vma, vmaddr) flushes one page
 *  - flush_tlb_range(mm, start, end) flushes a range of pages
 *
 * XXX we are brute-forcing this, it should use IPTE when possible.
 */

/*
 * Flush all tlb/hash table entries (except perhaps for those
 * mapping RAM starting at PAGE_OFFSET, since they never change).
 */
void
i370_flush_tlb_all(void)
{
#ifdef LATER
	read_lock(&tasklist_lock);
	for_each_task(tsk) {
		if (tsk->mm)
			_lctl_r1(...) /* set cr1 and then ?? */
	}
	read_unlock(&tasklist_lock);
#endif
	_ptlb();
}

/*
 * Flush all the (user) entries for the address space described
 * by mm.  We can't rely on mm->mmap describing all the entries
 * that might be in the hash table.
 */
void
i370_flush_tlb_mm(struct mm_struct *mm)
{
	printk ("i370_flush_tlb_mm pgd=%x\n", mm->pgd);
	_ptlb();
}

void
i370_flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
	// printk ("i370_flush_tlb_page for %lx\n", vmaddr);
	/* Should be doing a IPTE here ...!? */
	_ptlb();
}

void
i370_flush_tlb_range(struct mm_struct *mm, unsigned long start, unsigned
long end)
{
	printk ("i370_flush_tlb_range pgd=%x\n", mm->pgd);
	_ptlb();
}

int do_check_pgt_cache(int low, int high)
{
	return 0;
}

void __bad_pte(pmd_t *pmd)
{
	printk("Bad pmd in pte_alloc: %08lx\n", pmd_val(*pmd));
	pmd_val(*pmd) = (unsigned long) BAD_PAGETABLE;
}

/* XXX hack alert -- we can in fact fit four page tables onto
 * one page, and we should muck around in this routine to make this so.
 * but for now, we'll be slobs and use one page per page table.
 */
pte_t *i370_get_pte(pmd_t *pmd, unsigned long entry_index)
{
	pte_t *pte;

	/* entry index had better range between 0 and 255 */
	/* printk ("enter i370_get_pte addr_of_ste=%p dx=%ld\n", pmd, entry_index); */
	if (pmd_none(*pmd)) {
		pte = (pte_t *) __get_free_page(GFP_KERNEL);
		if (pte) {
			int i;
			pte_t *pe = pte;
			unsigned long ste = (unsigned long) pte;

			for (i=0; i<PTRS_PER_PTE; i++) {
				pte_clear (pe);
				pe ++;
			}
			ste &= SEG_PTO_MASK;
			ste |= 0xf;	/* length=(15+1)*16=256 */
			pmd_val(*pmd) = ste;
			return pte + entry_index;
		}
		pmd_val(*pmd) = (unsigned long)BAD_PAGETABLE;
		return NULL;
	}

	if (pmd_bad(*pmd)) {
		__bad_pte(pmd);
		return NULL;
	}
	return ((pte_t *) pmd_page(*pmd)) + entry_index;
}

/*
 * BAD_PAGE is the page that is used for page faults when linux
 * is out-of-memory. Older versions of linux just did a
 * do_exit(), but using this instead means there is less risk
 * for a process dying in kernel mode, possibly leaving a inode
 * unused etc..
 *
 * BAD_PAGETABLE is the accompanying page-table: it is initialized
 * to point to BAD_PAGE entries.
 *
 * ZERO_PAGE is a special page that is used for zero-initialized
 * data and COW.
 */
unsigned long empty_bad_page_table;
unsigned long empty_bad_page;
pte_t  bad_pte;

pte_t * __bad_pagetable(void)
{
	return (pte_t *) empty_bad_page_table;
}


pte_t __bad_page(void)
{
	return bad_pte;
}


/*
 * paging_init() sets up the page tables
 */
__initfunc(unsigned long paging_init(unsigned long start_mem,
                                     unsigned long end_mem))
{
	int i;
	pte_t *pte;
	/* printk ("paging init startmem=%lx endmem=%lx\n", start_mem, end_mem); */
	/*
	 * Grab some memory for bad_page and bad_pagetable to use.
	 * Clear out the bad page.  Initilialze the bad pagetable.
	 */
	empty_bad_page = PAGE_ALIGN(start_mem);
	memset ((void *)empty_bad_page, 0, PAGE_SIZE);
	bad_pte = pte_mkdirty(mk_pte(empty_bad_page, PAGE_SHARED));

	empty_bad_page_table = empty_bad_page + PAGE_SIZE;
	pte = (pte_t *) empty_bad_page_table;
	for (i=0; i<PTRS_PER_PTE; i++) {
		*pte = bad_pte;
		pte ++;
	}

	start_mem = empty_bad_page_table + PAGE_SIZE;

	/* free_area_init is in linux/mm/page_malloc.c and it
	 * sets up its arch-independent page tables*/
	start_mem = free_area_init(start_mem, end_mem);

	return start_mem;
}

/* ========================================================== */
/* copy in/out routines of various sorts */
/* XXX probably would get a performance boost by
 * using the 'LRA' instruction. (assuming cr1 is valid).
 */
/* XXX all wrong for page boundary crossings .... */


int
__copy_to_user (void * to, const void * from, unsigned long len)
{
	/* kernel page tables require no translation */
	if (current->mm->pgd == swapper_pg_dir) {
		memcpy (to,from,len);
		return 0;
	} else {
		unsigned long va, frm;
		va = (unsigned long) to;
		frm = (unsigned long) from;
		if (va < PAGE_SIZE) {
			printk(KERN_WARNING "copy_to_user null ptr va=0x%lx\n", va);
			show_regs (current->tss.regs);
			print_backtrace (current->tss.regs->irregs.r13);
			printk ("\nabove was user stack, the kernel stack shown below:\n");
			print_backtrace (_get_SP());
			i370_halt ();
		}
		while (len > 0) {
			unsigned long off = va & ~PAGE_MASK;
			unsigned long rlen = PAGE_SIZE - off;
			unsigned long ra;
			pte_t *pte;

			pte = find_pte (current->mm, va);
			if (!pte || pte_none(*pte)) {
				/* printk ("cpy_to_user make_pages_present at va=%lx\n", va); */
				make_pages_present (va, va+len);
				pte = find_pte (current->mm, va);
			}

			if (len < rlen) rlen = len;
			ra = pte_page (*pte) | off;
			/* printk ("cpy_to_user va=%lx ra=%lx\n", va, ra); */
			memcpy ((void *)ra, (void *)frm, rlen);
			len -= rlen;
			frm += rlen;
			va += rlen;
		}
	}
	return 0;
}

/* ========================================================== */

int
__copy_from_user (void * to, const void * from, unsigned long len)
{
	/* kernel page tables require no translation */
	if (current->mm->pgd == swapper_pg_dir) {
		memcpy (to,from,len);
		return 0;
	} else {
		unsigned long va, too;
		va = (unsigned long) from;
		too = (unsigned long) to;
		if (va < PAGE_SIZE) {
			printk(KERN_WARNING "copy_from_user null ptr va=0x%lx\n", va);
			show_regs (current->tss.regs);
			print_backtrace (current->tss.regs->irregs.r13);
			printk ("\nabove was user stack, the kernel stack shown below:\n");
			print_backtrace (_get_SP());
			i370_halt ();
		}
		while (len > 0) {
			unsigned long off = va & ~PAGE_MASK;
			unsigned long rlen = PAGE_SIZE - off;
			unsigned long ra;
			pte_t *pte;

			pte = find_pte (current->mm, va);
			if (!pte || pte_none(*pte)) {
				/* printk ("cpy_from_user make_pages_present at va=%lx\n", va); */
				make_pages_present (va, va+len);
				pte = find_pte (current->mm, va);
			}

			if (len < rlen) rlen = len;
			ra = pte_page (*pte) | off;
			/* printk ("cpy_from_user va=%lx ra=%lx\n", va, ra); */
			memcpy ((void *)too, (void *)ra, rlen);
			len -= rlen;
			too += rlen;
			va += rlen;
		}
	}
	return 0;
}

/* ========================================================== */

int __strncpy_from_user(char *dst, const char *src, long count)
{
	/* kernel page tables require no translation */
	if (current->mm->pgd == swapper_pg_dir) {
		long  lcl_count;
		lcl_count = strlen(src);
		if (count < lcl_count) lcl_count = count;
		strncpy (dst,src,count);
		return lcl_count;
	} else {
		unsigned long len, clen, va, too;
		len = count;
		clen = 0;
		va = (unsigned long) src;
		too = (unsigned long) dst;
		if (va < PAGE_SIZE)
			printk(KERN_WARNING "strncpy_from_user null ptr va=0x%lx\n", va);
		while (len > 0) {
			unsigned long off = va & ~PAGE_MASK;
			unsigned long rlen = PAGE_SIZE - off;
			unsigned long ra;
			unsigned long i=0;
			pte_t *pte;

			pte = find_pte (current->mm, va);
			if (!pte || pte_none(*pte)) {
				/* printk ("strncpy_from_user make_pages_present at va=%lx\n", va); */
				make_pages_present (va, va+count);
				pte = find_pte (current->mm, va);
			}
			if (len < rlen) rlen = len;
			ra = pte_page (*pte) | off;
			/* printk ("strncpy_from_user va=%lx ra=%lx\n", va, ra); */
			while (i<rlen) {
				if (0 == *(char *)(ra+i)) {
					i++;  /* copy null byte */
					break;
				}
				i++;
				clen ++;
			}
			memcpy ((void *)too, (void *)ra, i);
			len -= rlen;
			too += rlen;
			va += rlen;
			if (rlen != i) break; /* found null ? */
		}
		return clen;
	}
	return 0;
}

/* ========================================================== */

/* The following routines accept virtual addresses only:
 * clear_user
 * strlen_user
 *
 * The way that these routines are currently used in the kernel, one must
 * assume that their arguments are always a virtual addresses, and never
 * real addresses.   For example, create_elf_tables() calls strlen_user()
 * without first doing an appropriate set_fs(). Similarly, padzero in
 * load_elf_binary() does the same ...
 */

/* clear_user(): set a block of bytes to zero */
unsigned long
__clear_user(void *addr, unsigned long len)
{
	unsigned long va = (unsigned long) addr;

	/* if we are using the kernel page tables,
	 * there is no translation to be done */
	if (current->mm->pgd == swapper_pg_dir) {
		memset (addr, 0, len);
		return len;
	}
	while (len > 0) {
		unsigned long off = va & ~PAGE_MASK;
		unsigned long rlen = PAGE_SIZE - off;
		unsigned long ra;
		pte_t *pte;

		if (va < PAGE_SIZE)
			printk(KERN_WARNING "clear_user null ptr va=0x%lx\n", va);

		pte = find_pte (current->mm, va);
		if (!pte || pte_none(*pte)) {
			/* printk ("clear_user make_pages_present at va=%lx\n", va); */
			make_pages_present (va, va+len);
			pte = find_pte (current->mm, va);
		}
		if (len < rlen) rlen = len;
		ra = pte_page (*pte) | off;
		/* printk ("clear_user va=%lx ra=%lx len=%ld\n", va, ra, len); */
		memset ((void *)ra, 0, rlen);
		len -= rlen;
		va += rlen;
	}
	return len;
}

/* ========================================================== */

/*
 * strlen_user():  Return the size of a string (including the ending 0)
 * Return 0 for error.
 */

long
strlen_user(const char *str)
{
	int notdone = 1;
	unsigned long clen = 0;
	unsigned long va = (unsigned long) str;

	/* if we are using the kernel page tables,
	 * there is no translation to be done */
	if (current->mm->pgd == swapper_pg_dir) {
		return (strlen(str) + 1);
	}
	while (notdone) {
		unsigned long off = va & ~PAGE_MASK;
		unsigned long rlen = PAGE_SIZE - off;
		unsigned long ra;
		unsigned long i=0;
		pte_t *pte;

		if (va < PAGE_SIZE)
			printk(KERN_WARNING "strlen_user null ptr va=0x%lx\n", va);
		pte = find_pte (current->mm, va);
		if (!pte || pte_none(*pte)) {
			/* printk ("strlen_user make_pages_present at va=%lx\n", va); */
			make_pages_present (va,va);
			pte = find_pte (current->mm, va);
		}
		ra = pte_page (*pte) | off;
		/* printk ("strlen_user va=%lx ra=%lx\n", va, ra); */
		while (i<rlen) {
			if (0 == *(char *)(ra+i)) {
				notdone = 0;
				i++;  /* count null byte */
				break;
			}
			i++;
		}
		clen += i;
		va += rlen;
	}
	return clen;
}

/* ========================================================== */
/* put_user_data() will store data into address.  It is called with
 * len of 1,2 or 4 from uaccess.h, no other len's occur. */

void
put_user_data(long data, void *addr, long len)
{
	pte_t *pte;
	unsigned long va, ra, off;

	/* kernel page tables require no translation */
	if (current->mm->pgd == swapper_pg_dir) {
		if (1 == len) {
			*((char *) addr) = (char) data;
		} else
		if (2 == len) {
			*((short *) addr) = (short) data;
		} else
		if (4 == len) {
			*((int *) addr) = (int) data;
		}
		return;
	}

	/* find the page table entry for the addr */
/* XXX note we need to figure out some way of walking the
 * the pte in some SMP-safe fashion, which find_pte isn't ...
 * maybe mark the page w/ compare & swap?
 * note LRA (load real addr) is not enough ...
 */
	va = (unsigned long) addr;
	if (va < PAGE_SIZE)
		printk(KERN_WARNING "put_user_data null ptr va=0x%lx\n", va);
	pte = find_pte (current->mm, va);

	if (!pte || pte_none(*pte)) {
		/* printk ("put_user_data make_pages_present at va=%lx\n", va); */
		make_pages_present (va, va+len);
		pte = find_pte (current->mm, va);
	}
	/* printk ("put_user_data: va=%lx pte=%p pteval=%lx\n", va, pte, pte_val(*pte)); */

	/* put together the real address */
	off = va & ~PAGE_MASK;
	ra = (pte_val(*pte) & PAGE_MASK) | off;

	if (1 == len) {
		char *ca = (char *) ra;
		*ca = (char) (data & 0xff);
	}

	/* for length 2 or 4, data may be split over page boundary ... */
	else
	if (2 == len) {
		if (0xfff > off) {
			short *sa = (short *) ra;
			*sa = (short) (data & 0xffff);
		} else {
			char *ca = (char *) ra;
			*ca = (char) (data & 0xff);
			put_user_data (data>>8, (void *) (va+1), 1);
		}
	}

	else
	if (4 == len) {
		if (0xffd > off) {
			long *la = (long *) ra;
			*la = (long) data;
		} else
		if (0xffd == off) {
			short *sa = (short *) ra;
			char *ca = (char *) (ra+2);
			*sa = (short) (data & 0xffff);
			*ca = (char) ((data>>16) & 0xff);
			put_user_data (data>>24, (void *) (va+3), 1);
		} else
		if (0xffe == off) {
			short *sa = (short *) ra;
			*sa = (short) (data & 0xffff);
			put_user_data (data>>16, (void *) (va+2), 2);
		} else
		{
			char *ca = (char *) ra;
			*ca = (char) (data & 0xff);
			put_user_data (data>>8, (void *) (va+1), 3);
		}
	}

	/* length 3 is always safe, since only called from above */
	else
	if (3 == len) {
		short *sa = (short *) ra;
		char *ca = (char *) (ra+2);
		*sa = (short) (data & 0xffff);
		*ca = (char) ((data>>16) & 0xff);
	}
}

/* ============================ END OF FILE ============================== */
