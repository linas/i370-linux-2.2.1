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
 
int
do_page_fault(struct pt_regs *regs, unsigned long address,
              unsigned short pic_code)
{
 struct mm_struct *mm = current->mm;
 struct vm_area_struct * vma;
 unsigned long fixup, valid_addr;
 int write;
 unsigned short *ilc;
 
  /* printk ("do_page_fault addr=0x%lx, pic=%x\n", address, pic_code); */
  /*------------------------------------------------------------*/
  /* If we're in an interrupt or have no user                   */
  /* context, we must not take the fault..                      */
  /*------------------------------------------------------------*/
  if (in_interrupt() || mm == &init_mm)
       goto no_context;
  down(&mm->mmap_sem);
  valid_addr = address & MASK_TRXVALID;
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
   * It might be a null-pointer deref ...
   *
   * Note clear on what's going on here. This can happen for the 
   * user process stack, although note that that grows up, not down.
   * Note also, it should stact a 7fc00000 but sometimes starts at
   * 7fbfff00 which I think is wrong, but .. anyway, this are needs
   * work both here and in the elf loader.
   */
printk ("do_page_fault addr=0x%lx is below vma start=0x%lx\n", address, vma->vm_start);
  /* if (!(vma->vm_flags & VM_GROWSUP)) goto bad_area; */

  if (user_mode(regs)) {
       /*-----------------------------------------------------*/
       /*-----------------------------------------------------*/
printk ("do_page_fault user stack fault addr=0x%lx, frame=%lx\n", address, regs->irregs.r13);
       /* XXX hack alert see the corresponding stack XXXX in i370_start_thread() */
       /* XXX basically this is a really piss-poor attempt at stack verification */
       if (address < ((TASK_SIZE-I370_STACK_SIZE)-0x1000))
               goto bad_area;
  }
  if (expand_stack(vma, address))
       goto bad_area;
 
  /*----------------------------------------------------------*/
  /* Ok, we have a good vm_area for this memory access, so    */
  /* we can handle it..                                       */
  /*----------------------------------------------------------*/
good_area:
  write = 0;
  if (pic_code == PIC_PROTECTION) {
     if (!(vma->vm_flags & VM_WRITE))
        goto bad_area;
     if (!valid_addr)
        goto bad_area;
     write = 1;
     ilc   = PFX_PRG_CODE;
     regs->psw.addr -= *ilc;  /* Adjust the instruction ptr    */
  } else {
     /*-------------------------------------------------------*/
     /* Check if this page is allowed to be written to        */
     /*-------------------------------------------------------*/
     if (vma->vm_flags & VM_WRITE)
        write = 1;
  }
  /* printk(" handling mm fault on good page \n"); */
 
  /*----------------------------------------------------------*/
  /* If for any reason at all we couldn't handle the fault,   */
  /* make sure we exit gracefully rather than endlessly redo  */
  /* the fault.                                               */
  /*----------------------------------------------------------*/
  if (!handle_mm_fault(current, vma, address, write))
       goto do_sigbus;
  up(&mm->mmap_sem);
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
 
printk("sending SEGV to user proc: bad access to 0x%lx pic=%x\n", address, pic_code);
show_regs(regs);
print_backtrace (regs->irregs.r13);

       info.si_signo = SIGSEGV;
       info.si_code  = SEGV_MAPERR;
       info.si_addr  = (void *) address;
       force_sig_info(SIGSEGV, &info, current);
       return 1;
  }
 
no_context:
  /*----------------------------------------------------------*/
  /* Are we prepared to handle this kernel fault?   (not yet!)*/
  /*----------------------------------------------------------*/
/*if ((fixup = search_exception_table(regs->psw.addr)) != 0) {
       regs->psw.addr = fixup | 0x80000000;
       return -1;
  }
*//*----------------------------------------------------------*/
  /* Oops. The kernel tried to access some bad page. We'll    */
  /* have to terminate things "with extreme prejudice".       */
  /*----------------------------------------------------------*/
  if ((unsigned long) address < PAGE_SIZE) {
       printk(KERN_ALERT "Unable to handle kernel NULL pointer dereference");
  }
  else
       printk(KERN_ALERT "Unable to handle kernel access");
  printk(" at virtual address %08lx\n",address);
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
