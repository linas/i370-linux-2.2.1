
/* 
 * XXX almost everythin in this file is wrong ...
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


__initfunc(void mem_init(unsigned long start_mem, unsigned long end_mem))
{
   unsigned long addr;
   int lowpages = 0;
   int codepages = 0;
   int datapages = 0;
   int initpages = 0;
   int initrdpages = 0;

   printk ("enter mem init\n");
   end_mem &= PAGE_MASK;
   high_memory = (void *) end_mem;
   max_mapnr = MAP_NR(high_memory);

   /* mark usable pages in the mem_map[] */
   start_mem = PAGE_ALIGN(start_mem);
   num_physpages = max_mapnr;      /* RAM is assumed contiguous */

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

pte_t *i370_get_pte(pmd_t *pmd, unsigned long offset)
{
   pte_t *pte;

   printk ("enter i370_get_pte \n");
   if (pmd_none(*pmd)) {
      pte = (pte_t *) __get_free_page(GFP_KERNEL);
      if (pte) {
         clear_page((unsigned long)pte);
         pmd_val(*pmd) = (unsigned long)pte;
         return pte + offset;
      }
      pmd_val(*pmd) = (unsigned long)BAD_PAGETABLE;
      return NULL;
   }

   if (pmd_bad(*pmd)) {
      __bad_pte(pmd);
      return NULL;
   }
   return (pte_t *) pmd_page(*pmd) + offset;
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

pte_t * __bad_pagetable(void)
{
   __clear_user((void *)empty_bad_page_table, PAGE_SIZE);
   return (pte_t *) empty_bad_page_table;
}


pte_t __bad_page(void)
{
   __clear_user((void *)empty_bad_page, PAGE_SIZE);
   return pte_mkdirty(mk_pte(empty_bad_page, PAGE_SHARED));
}


/*
 * paging_init() sets up the page tables
 */
__initfunc(unsigned long paging_init(unsigned long start_mem, 
                                     unsigned long end_mem))
{
   printk ("paging init\n");
   /*
    * Grab some memory for bad_page and bad_pagetable to use.
    */
   empty_bad_page = PAGE_ALIGN(start_mem);
   empty_bad_page_table = empty_bad_page + PAGE_SIZE;
   start_mem = empty_bad_page + 2 * PAGE_SIZE;

   /* free_area_init is in linux/mm/page_malloc.c and it 
    * sets up its arch-independent page tables*/
   start_mem = free_area_init(start_mem, end_mem);

   return start_mem;
}

void set_context(int context) {}  


