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
/************************************************************/
/*                                                          */
/*                       DEFINES                            */
/*                       -------                            */
/*                                                          */
/************************************************************/
 
/*=============== End of Defines ===========================*/
 
/************************************************************/
/*                                                          */
/*                INCLUDE STATEMENTS                        */
/*                ------------------                        */
/*                                                          */
/************************************************************/
 
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/asm.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/siginfo.h>
#include <asm/semaphore.h>
 
/*================== End of Include Statements =============*/
 
/************************************************************/
/*                                                          */
/*                TYPE DEFINITIONS                          */
/*                ----------------                          */
/*                                                          */
/************************************************************/
 
/*================== End of Type Definitions ===============*/
 
/************************************************************/
/*                                                          */
/*               FUNCTION PROTOTYPES                        */
/*               -------------------                        */
/*                                                          */
/************************************************************/
 
/*================== End of Prototypes =====================*/
 
/************************************************************/
/*                                                          */
/*             GLOBAL VARIABLE DECLARATIONS                 */
/*             ----------------------------                 */
/*                                                          */
/************************************************************/
 
 
/*============== End of Variable Declarations ==============*/
 
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
 
printk ("do_page_fault addr=0x%x, pic=%x\n", address, pic_code);
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

  /* If we are here, then the address was below the vma start.
   * In that case, we better be dealing with the stack.
   * (The current i370 ELF stack grows down)
   * XXX there are good reasons to make the elf stack grow up.
   */
printk ("do_page_fault stack fault addr=0x%x\n", address);
  if (!(vma->vm_flags & VM_GROWSDOWN))
       goto bad_area;
  if (user_mode(regs)) {
       /*-----------------------------------------------------*/
       /* Accessing the stack below usp is always a bug.      */
/* ??? XXX currently, stack grows down */
       /*-----------------------------------------------------*/
printk ("do_page_fault usermode stack fault addr=0x%x, stackbot=%x\n", address, regs->irregs.r13);
/*
       if (address < regs->irregs.r13)
               goto bad_area;
*/
  }
  if (expand_stack(vma, address))
       goto bad_area;
 
  /*----------------------------------------------------------*/
  /* Ok, we have a good vm_area for this memory access, so    */
  /* we can handle it..                                       */
  /*----------------------------------------------------------*/
good_area:
printk(" handling mm fault on good page \n");
  write = 0;
  if (pic_code == PIC_PROTECTION) {
     if (!(vma->vm_flags & VM_WRITE))
        goto bad_area;
     if (!valid_addr)
        goto bad_area;
     write++;
  }
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
 
printk(" sending SEGV to user proc: here's the reg dump & backtrace:\n");
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
