
#include <linux/init.h>

#include <asm/atomic.h>

atomic_t next_mmu_context; 

__initfunc(void mem_init(unsigned long start_mem, unsigned long end_mem))
{
}

/*
 * paging_init() sets up the page tables
 */
__initfunc(unsigned long paging_init(unsigned long start_mem, unsigned long
end_mem))
{
   return 0;
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
local_flush_tlb_all(void)
{
 //        asm volatile ("tlbia" : : );
}



/*
 * Flush all the (user) entries for the address space described
 * by mm.  We can't rely on mm->mmap describing all the entries
 * that might be in the hash table.
 */
void
local_flush_tlb_mm(struct mm_struct *mm)
{
/*
        mm->context = NO_CONTEXT;
        if (mm == current->mm)
                activate_context(current);
*/
}

void
local_flush_tlb_page(struct vm_area_struct *vma, unsigned long vmaddr)
{
#if 0
        if (vmaddr < TASK_SIZE)
                flush_hash_page(vma->vm_mm->context, vmaddr);
        else
                flush_hash_page(0, vmaddr);
#endif
}

