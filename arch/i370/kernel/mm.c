
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
#include <asm/pgtable.h>

atomic_t next_mmu_context; 
struct pgtable_cache_struct quicklists;
extern void *__init_begin, *__init_end;
extern void *_text, *_etext,  *_stext;  


__initfunc(void mem_init(unsigned long start_mem, unsigned long end_mem))
{
   unsigned long addr;
   int lowpages = 0;
   int codepages = 0;
   int datapages = 0;
   int initpages = 0;

   printk ("enter mem init\n");
   end_mem &= PAGE_MASK;
   high_memory = (void *) end_mem;
   max_mapnr = MAP_NR(high_memory);

   /* mark usable pages in the mem_map[] */
   start_mem = PAGE_ALIGN(start_mem);
   num_physpages = max_mapnr;      /* RAM is assumed contiguous */

   for (addr = PAGE_OFFSET; addr < end_mem; addr += PAGE_SIZE)
   {
     set_bit(PG_reserved, &mem_map[MAP_NR(addr)].flags);                
   }

   /* mark the first page RO since all vectors have been set up by now */
   _sske (0x0, KTEXT_STORAGE_KEY);

   for (addr = PAGE_OFFSET; addr < end_mem; addr += PAGE_SIZE) 
   {
      if (PageReserved(mem_map + MAP_NR(addr))) 
      {
         if (addr < (ulong) _text)
         {
            lowpages++;
         }
         else if (addr < (ulong) _etext)
         {
            _sske (addr, KTEXT_STORAGE_KEY);
            codepages++;
         }
         else if (addr >= (ulong) __init_begin
               && addr < (ulong) __init_end)
         {
            initpages++;
         }
         else if (addr < (ulong) start_mem)
         {
            _sske (addr, KDATA_STORAGE_KEY);
            datapages++;
         }
         continue;
      }
      atomic_set(&mem_map[MAP_NR(addr)].count, 1);
#ifdef CONFIG_BLK_DEV_INITRD
      if (!initrd_start ||
           addr < (initrd_start & PAGE_MASK) || addr >= initrd_end)
#endif /* CONFIG_BLK_DEV_INITRD */
       free_page(addr);
   }

   printk("Memory: %luk available "
          "(%dk kernel code, %dk data, %dk init) "
          "[%08x,%08lx]\n",
     (unsigned long) nr_free_pages << (PAGE_SHIFT-10),
     codepages << (PAGE_SHIFT-10),
     datapages << (PAGE_SHIFT-10),
     initpages << (PAGE_SHIFT-10),
     PAGE_OFFSET, end_mem);
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
//        struct task_struct *tsk;

        printk("mmu_context_overflow\n");
#if 0
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
 * since the hardware hash table functions as an extension of the
 * tlb as far as the linux tables are concerned, flush it too.
 *    -- Cort
 */

/*
 * Flush all tlb/hash table entries (except perhaps for those
 * mapping RAM starting at PAGE_OFFSET, since they never change).
 */
void
i370_flush_tlb_all(void)
{
 //        asm volatile ("tlbia" : : );
}



/*
 * Flush all the (user) entries for the address space described
 * by mm.  We can't rely on mm->mmap describing all the entries
 * that might be in the hash table.
 */
void
i370_flush_tlb_mm(struct mm_struct *mm)
{
/*
        mm->context = NO_CONTEXT;
        if (mm == current->mm)
                activate_context(current);
*/
}

void
i370_flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
#if 0
        if (vmaddr < TASK_SIZE)
                flush_hash_page(vma->vm_mm->context, vmaddr);
        else
                flush_hash_page(0, vmaddr);
#endif
}

/*
 * for each page addr in the range, call MMU_invalidate_page()
 * if the range is very large and the hash table is small it might be
 * faster to do a search of the hash table and just invalidate pages
 * that are in the range but that's for study later.
 * -- Cort
 */
void
i370_flush_tlb_range(struct mm_struct *mm, unsigned long start, unsigned
long end)
{
#if 0
        start &= PAGE_MASK;

        if (end - start > 20 * PAGE_SIZE)
        {
                flush_tlb_mm(mm);
                return;
        }

        for (; start < end && start < TASK_SIZE; start += PAGE_SIZE)
        {
                flush_hash_page(mm->context, start);
        }
#endif
}

int do_check_pgt_cache(int low, int high)
{
  return 0;
}

/*
 * Flush a particular page from the DATA cache
 * Note: this is necessary because the instruction cache does *not*
 * snoop from the data cache. XXXX
 *
 *      void flush_page_to_ram(void *page)
 */
void flush_page_to_ram(unsigned long page) {}
void flush_icache_range(unsigned long a, unsigned long b) {}


void __bad_pte(pmd_t *pmd)
{
   printk("Bad pmd in pte_alloc: %08lx\n", pmd_val(*pmd));
   pmd_val(*pmd) = (unsigned long) BAD_PAGETABLE;
}

pte_t *get_pte_slow(pmd_t *pmd, unsigned long offset)
{
   printk ("get_pte_slow \n");
return 0x0;
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
   _sske (start_mem, 0x00);  // just for grins

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

