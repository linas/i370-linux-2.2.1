
/* 
 * XXX some of what's in this file is very wrong ...
 */

#include <linux/blk.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/swap.h>

#include <asm/asm.h>
#include <asm/atomic.h>
#include <asm/mmu.h>
#include <asm/mmu_context.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>

extern unsigned long free_area_init(unsigned long, unsigned long);


atomic_t next_mmu_context; 
extern char __init_text_begin[], __init_text_end[];
extern char __init_data_begin[], __init_data_end[];
extern char _text[], _etext[];  
extern unsigned char CPUID[8];

/* mem_init() will put the kernel text pages into a different
 * storage key than the data pages, effectively rendering them read-only.
 * It then changes the key under which the kernel executes.  Remaining 
 * area is marked as available for allocatin by the Linux kernel.
 */

__initfunc(void mem_init(unsigned long start_mem, unsigned long end_mem))
{
   unsigned long addr;
   int lowpages = 0;
   int codepages = 0;
   int datapages = 0;
   int initpages = 0;
   int initrdpages = 0;

   printk ("enter mem init startmem=0x%x endmem=0x%x\n", start_mem, end_mem);
   end_mem &= PAGE_MASK;
   high_memory = (void *) end_mem;
   max_mapnr = MAP_NR(high_memory);

   /* mark usable pages in the mem_map[] */
   start_mem = PAGE_ALIGN(start_mem);
   num_physpages = max_mapnr;      /* RAM is assumed contiguous */
 
#ifdef CONFIG_BLK_DEV_INITRD
#ifdef CONFIG_VM
	/*-------------------------------------------------------*/
	/* If we are running under VM then we can load the RAM   */
	/* disk from a shared segment (isn't VM wonderful!)      */
	/*-------------------------------------------------------*/
	if (CPUID[0] == 0xff)
	{
		static char NSSID[8] = {0xd9, 0xc1, 0xd4, 0xc4,
		                        0xc9, 0xe2, 0xd2, 0x40};
		static struct {
			  double alignment;
			  char RAMNSS[8];
		} NSS;
 
		memcpy(NSS.RAMNSS, NSSID, 8);
		__asm__ __volatile__ ("
			LA	r14,%2;
			LA	r15,4(0);
			SLR	r0,r0;
			EX	r0,=X'83EF0064';
			ST	r14,%0;
			ST	r15,%1"
			: "=m" (initrd_start), "=m" (initrd_end)
			: "m" (*NSS.RAMNSS)
			: "memory", "r0", "r14", "r15");
 
		printk("RAMDISK NSS loaded\n");
	}
#endif /* CONFIG_VM */
#endif /* CONFIG_BLK_DEV_INITRD */

   /* mark the first page RO since all vectors have been set up by now */
   _sske (KTEXT_STORAGE_KEY, 0x0);

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

   /* take away write privledges from text pages. */
   _spka (KDATA_STORAGE_KEY);

   printk("Memory: %luk available "
          "(%dk code, %dk data, %dk init)\n",
     (unsigned long) nr_free_pages << (PAGE_SHIFT-10),
     codepages << (PAGE_SHIFT-10),
     datapages << (PAGE_SHIFT-10),
     initpages << (PAGE_SHIFT-10));

#ifdef CONFIG_BLK_DEV_INITRD
   printk("Init Ramdisk: %luk  [%08lx,%08lx]\n",
     initrdpages << (PAGE_SHIFT-10),
     initrd_start, initrd_end);
#endif /* CONFIG_BLK_DEV_INITRD */
}

__initfunc(void free_initmem(void))
{
  printk ("do da free_initmem ding \n");
} 

void si_meminfo(struct sysinfo *val)
{ }

      
/*
 * The context counter has overflowed.
 * We set mm->context to NO_CONTEXT for all mm's in the system.
 * We assume we can get to all mm's by looking as tsk->mm for
 * all tasks in the system.
 */
void
mmu_context_overflow(void)
{
        // struct task_struct *tsk;

        printk("mmu_context_overflow\n");

#ifdef JUNK
        read_lock(&tasklist_lock);
        for_each_task(tsk) {
                if (tsk->mm)
                        tsk->mm->context = NO_CONTEXT;
        }
        read_unlock(&tasklist_lock);
        flush_hash_segments(0x10, 0xffffff);
        atomic_set(&next_mmu_context, 0);
        /* make sure current always has a context */
        current->mm->context = MUNGE_CONTEXT(atomic_inc_return(&next_mmu_context));
        set_context(current->mm->context);
#endif
}


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
	printk ("i370_flush_tlb_mm\n");
	_ptlb();
/*
        mm->context = NO_CONTEXT;
        if (mm == current->mm)
                activate_context(current);
*/
}

void
i370_flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
	printk ("i370_flush_tlb_page\n");
	/* should be doing a IPTE here ... */
	_ptlb();
}

void
i370_flush_tlb_range(struct mm_struct *mm, unsigned long start, unsigned
long end)
{
	printk ("i370_flush_tlb_range\n");
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
	printk ("enter i370_get_pte addr_of_ste=%x dx=%d\n", pmd, entry_index);
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
	printk ("paging init startmem=%x endmem=%x\n", start_mem, end_mem);
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

void set_context(int context) {}  

/* ========================================================== */
/* copy in/out routines of various sorts */
/* XXX probably would get a performance boost by 
 * using the 'LRA' instruction. (assuming cr1 is valid).
 */


int
__copy_to_user (void * to, const void * from, unsigned long len)
{
	if (__kernel_ok) {
		memcpy (to,from,len);
		return 0;
	} else {
		unsigned long va, frm;
		va = (unsigned long) to;
		frm = (unsigned long) from;
		while (len > 0) {
			pte_t *pte;
			pte = find_pte (current->mm, va);
			if (pte_none(*pte)) {
				printk ("Error: copy_to_user: unmaped page \n");
				i370_halt();
			} else {
				unsigned long off = va & ~PAGE_MASK;
				unsigned long rlen = PAGE_SIZE - off;
				unsigned long ra;
				if (len < rlen) rlen = len;
				ra = pte_page (*pte) | off;
				printk ("cpy_to_user va=%x ra=%x\n", va, ra);
				memcpy ((void *)ra, (void *)frm, rlen);
				len -= rlen;
				frm += rlen;
				va += rlen;
			}
		}
	}
	return 1;
}

int
__copy_from_user (void * to, const void * from, unsigned long len)
{
	if (__kernel_ok) {
		memcpy (to,from,len);
		return 0;
	} else {
		unsigned long va, too;
		va = (unsigned long) from;
		too = (unsigned long) to;
		while (len > 0) {
			pte_t *pte;
			pte = find_pte (current->mm, va);
			if (pte_none(*pte)) {
				printk ("Error: copy_from_user: unmaped page \n");
				i370_halt();
			} else {
				unsigned long off = va & ~PAGE_MASK;
				unsigned long rlen = PAGE_SIZE - off;
				unsigned long ra;
				if (len < rlen) rlen = len;
				ra = pte_page (*pte) | off;
				printk ("cpy_from_user va=%x ra=%x\n", va, ra);
				memcpy ((void *)too, (void *)ra, rlen);
				len -= rlen;
				too += rlen;
				va += rlen;
			}
		}
	}
	return 1;
}

int __strncpy_from_user(char *dst, const char *src, long count)
{

	if (__kernel_ok) {
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
		while (len > 0) {
			pte_t *pte;
			pte = find_pte (current->mm, va);
			if (pte_none(*pte)) {
				printk ("Error: strncpy_from_user: unmaped page \n");
				i370_halt();
			} else {
				unsigned long off = va & ~PAGE_MASK;
				unsigned long rlen = PAGE_SIZE - off;
				unsigned long ra;
				unsigned long i=0;
				if (len < rlen) rlen = len;
				ra = pte_page (*pte) | off;
				printk ("strncpy_from_user va=%x ra=%x\n", va, ra);
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
		}
		return clen;
	}
	return 0;
}

/* The following routines accept virtual addresses only:
 * clear_user
 * strlen_user
 *
 * The way tht these routines are curently used in the kernel, one must 
 * assume that thier arguments are always a virtual addresses, and never 
 * real addresses.   For example, create_elf_tables() calls strlen_user()
 * without first doing an appropriate set_fs(). Similarly, padzero in
 * load_elf_binary() does the same ...
 */

unsigned long __clear_user(void *addr, unsigned long len)
{
	/* set a block of bytes to zero */  
	unsigned long va;
	va = (unsigned long) addr;
	while (len > 0) {
		pte_t *pte;
		pte = find_pte (current->mm, va);
		if (pte_none(*pte)) {
			printk ("Error: clear_user: unmaped page \n");
			i370_halt();
		} else {
			unsigned long off = va & ~PAGE_MASK;
			unsigned long rlen = PAGE_SIZE - off;
			unsigned long ra;
			if (len < rlen) rlen = len;
			ra = pte_page (*pte) | off;
			printk ("clear_user va=%x ra=%x len=%d\n", va, ra, len);
			memset ((void *)ra, 0, rlen);
			len -= rlen;
			va += rlen;
		}
	}
}

/*
 * strlen_user():  Return the size of a string (including the ending 0)
 * Return 0 for error.  
 */

long strlen_user(const char *str)
{
	unsigned long clen, va;
	int notdone = 1;
	clen = 0;
	va = (unsigned long) str;
	while (notdone) {
		pte_t *pte;
		pte = find_pte (current->mm, va);
		if (pte_none(*pte)) {
			printk ("Error: strncpy_from_user: unmaped page \n");
			i370_halt();
		} else {
			unsigned long off = va & ~PAGE_MASK;
			unsigned long rlen = PAGE_SIZE - off;
			unsigned long ra;
			unsigned long i=0;
			ra = pte_page (*pte) | off;
			printk ("strlen_user va=%x ra=%x\n", va, ra);
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
	}
	return clen;
}

/* put_user_data() will only be called with len of 1,2 or 4 
 * from uaccess.h */
void 
put_user_data(long data, void *addr, long len)
{
	pte_t *pte;
	unsigned long va, ra, off;

	/* find the page table entry for the addr */
/* XXX note we need to figure out some way of walking the 
 * the pte in some SMP-safe fashion, which find_pte isn't ...
 * maybe mark the page w/ compare & swap?
 * note LRA (load real addr) is not enough ...
 */
	va = (unsigned long) addr;
	pte = find_pte (current->mm, va);
	printk ("put_user_data: va=%x pte=%x = %x\n", va, pte, pte_val(*pte));

	if (pte_none(*pte)) {
		printk ("put_user_data: unmaped page \n");
		i370_halt();
		/* XXX set this up correctly ... */
		/* handle_mm_fault (current, ... ,1); */
		/* make_pages_present ...*/
	}

	/* put together the real address */
	off = va & ~PAGE_MASK;
	ra = (pte_val(*pte) & PAGE_MASK) | off;

	if (1 == len) {
		char *ca = (char *) ra;
		*ca = (char) (data & 0xff);
	}

	/* for length 2 or 4, data may be split over page boundry ... */
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
