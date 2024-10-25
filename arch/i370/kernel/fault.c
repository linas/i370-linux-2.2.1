/************************************************************/
/*                                                          */
/* Module ID  - fault.c                                     */
/*                                                          */
/* Function   - Handle page fault conditions for S/390 port */
/*              of Linux.                                   */
/*                                                          */
/*                                                          */
/* Parameters - (1) Parmname.                               */
/*                  Description - ......................... */
/*                  ....................................... */
/*                  Access - Read/Write/Update.             */
/*                                                          */
/*              (2) Parmname.                               */
/*                  Description - ......................... */
/*                  ....................................... */
/*                  Access - Read/Write/Update.             */
/*                                                          */
/*                                                          */
/* Called By  - Kernel.                                     */
/*                                                          */
/*                                                          */
/* Notes      - (1) ....................................... */
/*                                                          */
/*              (2) ....................................... */
/*                                                          */
/*                                                          */
/* Name       - Neale Ferguson.                             */
/* Date       - September, 1999.                            */
/*                                                          */
/*                                                          */
/* Associated    - (1) Refer To ........................... */
/* Documentation                                            */
/*                 (2) Refer To ........................... */
/*                                                          */
/************************************************************/


#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>

#include <asm/asm.h>
#include <asm/current.h>
#include <asm/pgtable.h>
#include <asm/semaphore.h>
#include <asm/siginfo.h>
#include <asm/system.h>
#include <asm/uaccess.h>


/************************************************************/
/*                                                          */
/*                     M A I N L I N E                      */
/*                     ---------------                      */
/*                                                          */
/* This routine handles page faults.  It determines the     */
/* problem and then passes it off to one of the appropriate */
/* routines.                                                */
/*                                                          */
/* pic_code:                                                */
/* - 0x0010 = Segment translation exception (no page)       */
/* - 0x0011 = Page translation exception (no page)          */
/* - 0x0004 = Protection exception (write into r/o)         */
/*                                                          */
/* If this routine detects a bad access, it returns 1,      */
/* otherwise it returns 0.                                  */
/************************************************************/
#if DEBUG
static int retry = 0; /* temporary debugging hack to avoid inf recursion. */
#endif

int
do_page_fault(struct pt_regs *regs, unsigned long address,
              unsigned short pic_code)
{
  struct mm_struct *mm = current->mm;
  struct vm_area_struct * vma;
  int write;
  unsigned short *ilc;
  cr1_t curr_cr1;

  /* printk ("do_page_fault addr=0x%lx, pic=%x\n", address, pic_code); */
  /*------------------------------------------------------------*/
  /* If we're in an interrupt or have no user                   */
  /* context, we must not take the fault..                      */
  /*------------------------------------------------------------*/
  if (in_interrupt() || mm == &init_mm)
       goto no_context;

  lock_kernel();
  down(&mm->mmap_sem);

  /* Sanity check: cr1 should be set by fork, clone and switch_to */
  curr_cr1.raw = _stctl_r1();
  if (curr_cr1.raw != current->tss.cr1.raw ||
      (curr_cr1.raw & MASK_TRXADDR) != ((unsigned long) mm->pgd) ||
      (void *) mm->pgd != (void *) current->tss.pg_tables) {
     printk("BUG: unexpected cr1. Have: %lx  Want: %lx pgd=%p tables=%p\n",
            curr_cr1.raw, current->tss.cr1.raw,
            mm->pgd, current->tss.pg_tables);

     current->tss.cr1.bits.psto = ((unsigned long) mm->pgd) >> 12;
     current->tss.cr1.bits.pstl = 127;
     _lctl_r1(current->tss.cr1.raw);

     /* Well, lets try to figure out whose fault this is. */
     show_regs(current->tss.regs);
     print_backtrace(_get_SP());
     i370_halt();
  }

  address &= MASK_TRXADDR;
  vma = find_vma(mm, address);
  if (!vma)
       goto bad_area;
  if (vma->vm_flags & VM_IO)
       goto bad_area;
  if (vma->vm_start <= address)
       goto good_area;

  if ((pic_code == PIC_PROTECTION) && address < 0x200) {
       printk ("do_page_fault null pointer deref ptr=0x%lx\n", address);
       goto bad_area;
  }

  /* If we are here, then the address was below the vma start.
   * Three ways this happens; a wild pointer in the kernel, a
   * null-pointer deref in userland, or a user-space stack touch.
   */
  if (!user_mode(regs)) {
       printk ("do_page_fault addr=0x%lx is below vma start=0x%lx\n",
               address, vma->vm_start);
       goto bad_area;
  }

  /*-----------------------------------------------------*/
  if (user_mode(regs)) {
       /* The only valid access is if the user is touching the stack.
        * The stack is (currently) between 7f800000 and 0x7ffff000.
        * (A 8 MB stack). See i370_start_thread() for details. Must
        * match what is there. Normal operation means
        * regs->irregs.r13 is above this.
        */
       unsigned long frame_base = STACK_TOP - I370_STACK_SIZE;
       if (address < frame_base) goto bad_area;

       /* The first time the user touches the stack, the expand_stack
        * will move vma->vm_start down by 8MBytes (to 7f800000) and
        * that's that. I guess this is harmless, since only the vma
        * grew, not actual memory consumption. In this kernel, the vma
        * can only ever grow down, never up, so it seems like there's
        * nothing to be done.
        */
       if (expand_stack(vma, address))
            goto bad_area;
  }

  /*----------------------------------------------------------*/
  /* Ok, we have a good vm_area for this memory access, so    */
  /* we can handle it..                                       */
  /*----------------------------------------------------------*/
good_area:
  write = 0;

  /* Protection might be because page is read-only, or because
   * the key is wrong.  */
  if (pic_code == PIC_PROTECTION) {
#if DEBUG
     printk("Protection fault on %lx  VM_WRITE=%x user=%x\n",
            address, (vma->vm_flags & VM_WRITE), user_mode(regs));
     printk("Segment table at %lx\n", mm->pgd);
     pte_t * ptep = find_pte(mm, address);
     printk("PTE location=%lx PTE entry=%lx\n", ptep, pte_val(*ptep));
     printk("Protection key=%lx\n", _iske(pte_page(*ptep)));
     printk("Control reg cr0=%lx\n", _stctl_r0());
     printk("Control reg cr1=%lx\n", _stctl_r1());
     printk("-------------------\n");

     retry++;
     if (6 < retry) goto bad_area;
#endif

     if (!(vma->vm_flags & VM_WRITE))
        goto bad_area;

     write = 1;
     ilc   = (unsigned short *) PFX_PRG_CODE;
     regs->psw.addr -= *ilc;  /* Adjust the instruction ptr    */
  } else {
     /*-------------------------------------------------------*/
     /* Check if this page is allowed to be written to        */
     /*-------------------------------------------------------*/
     if (vma->vm_flags & VM_WRITE)
        write = 1;
  }
  /* printk("Fault %s/%d good page addr=%x PSW flags: %08lX PSW addr: %08lX\n",
         current->comm, current->pid,
         address, regs->psw.flags, regs->psw.addr); */

  /*----------------------------------------------------------*/
  /* If for any reason at all we couldn't handle the fault,   */
  /* make sure we exit gracefully rather than endlessly redo  */
  /* the fault.                                               */
  /*----------------------------------------------------------*/
  if (!handle_mm_fault(current, vma, address, write))
       goto do_sigbus;
  up(&mm->mmap_sem);
  unlock_kernel();

  return 0;

/*------------------------------------------------------------*/
/* Something tried to access memory that isn't in our memory  */
/* map. Fix it, but check if it's kernel or user first. User  */
/* mode accesses just cause a SIGSEGV.                        */
/*------------------------------------------------------------*/
bad_area:
  up(&mm->mmap_sem);
  if (user_mode(regs)) {
       siginfo_t info;

       /* If /sbin/init faults, there is nothing we can do. */
       /* We can't kill it. We have to halt. */
       if (1 == current->pid) {
          printk("Init process (%s:%d) bad access to 0x%lx pic=%x\n",
                 current->comm, current->pid, address, pic_code);
          show_regs(regs);
          print_backtrace (regs->irregs.r13);
          i370_halt();
       }

#ifdef DEBUG
       printk("do_page_fault: send SEGV to %s/%d: bad access 0x%lx pic=%x\n",
               current->comm, current->pid, address, pic_code);
       show_regs(regs);
       print_backtrace (regs->irregs.r13);
#endif

       /* Goodbye, cruel world! */
       do_exit(SIGSEGV);
  }

no_context:
  /*----------------------------------------------------------*/
  /* Are we prepared to handle this kernel fault?   (not yet!)*/
  /*----------------------------------------------------------*/
/*if ((fixup = search_exception_table(regs->psw.addr)) != 0) {
       regs->psw.addr = fixup | 0x80000000;
       return -1;
  }
*/
  /*----------------------------------------------------------*/
  /* Oops. The kernel tried to access some bad page. We'll    */
  /* have to terminate things "with extreme prejudice".       */
  /*----------------------------------------------------------*/
  if ((unsigned long) address < PAGE_SIZE) {
    printk(KERN_ALERT
        "Kernel NULL pointer dereference at address %08lx\n", address);
     show_regs(regs);
     print_backtrace (_get_SP());
     i370_halt();
  }

  printk(KERN_ALERT
     "Unable to handle kernel access at address %08lx\n", address);
  show_regs(regs);
  print_backtrace (regs->irregs.r13);
  i370_halt();

/*-------------------------------------------------------------*/
/* We ran out of memory, or some other thing happened to us    */
/* that made us unable to handle the page fault gracefully.    */
/*-------------------------------------------------------------*/
do_sigbus:
  up(&mm->mmap_sem);
  /*
   * Send a sigbus, regardless of whether we were in kernel
   * or user mode.
   */
  force_sig(SIGBUS, current);
  /* Kernel mode? Handle exceptions or die */
  if (!(user_mode(regs)))
       goto no_context;
  return 1;
}

/*===================== End of Mainline ====================*/
