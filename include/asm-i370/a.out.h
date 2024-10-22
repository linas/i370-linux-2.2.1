#ifndef __I370_A_OUT_H__
#define __I370_A_OUT_H__

/* The stack top will be at the very highest possible memory 
   on the 31-bit machines: at 2GB */
#define STACK_TOP TASK_SIZE

/* The i370 stack will grow upwards; thus the mm stack flags need
    to indicate this */
#define VM_STACK_FLAGS 0x0277

/* Max stack that a user can get. Currently set to 8 MBytes,
   because this is identical to _STK_LIM set in sched.h.
   These must be kept in sync (by hand). */
#define I370_STACK_SIZE 0x800000  /* = _STK_LIM */

/* All user stacks start here. Anything above this is always a user
 * stack. Currently equals 0x7f800000 */
#define I370_STACK_BASE (STACK_TOP - I370_STACK_SIZE)

struct exec
{
  unsigned long a_info;		/* Use macros N_MAGIC, etc for access */
  unsigned a_text;		/* length of text, in bytes */
  unsigned a_data;		/* length of data, in bytes */
  unsigned a_bss;		/* length of uninitialized data area for file, in bytes */
  unsigned a_syms;		/* length of symbol table data in file, in bytes */
  unsigned a_entry;		/* start address */
  unsigned a_trsize;		/* length of relocation info for text, in bytes */
  unsigned a_drsize;		/* length of relocation info for data, in bytes */
};


#define N_TRSIZE(a)	((a).a_trsize)
#define N_DRSIZE(a)	((a).a_drsize)
#define N_SYMSIZE(a)	((a).a_syms)

#endif
